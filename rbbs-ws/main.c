/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <libwebsockets.h>

#include "buffer.h"

#define LOCAL_PRINT
#define DEBUG
//#define INPUT_CR_TO_CRLF

#define INLINE inline __attribute__((always_inline))
#define UNUSED __attribute__((unused))

static struct lws *wsi_global;
static int serial;
static RingBuffer *from_ws_buf;
static int baudrate = 115200;
static char* device;

// UART to WS buffer doesn't use a RingBuffer, since we need to keep the 
// LWS_PRE preamble for the websocket write anyway we may as well just 
// keep a buffer after that. We don't bother treating it as a ring buffer,
// if we can't send within the time it takes to fill, something is wrong anyhow...
#define OUT_BUFFER_SIZE     256
static unsigned char ch[LWS_PRE + OUT_BUFFER_SIZE];
static int chptr;

struct lws_context *context;

#ifdef INPUT_CR_TO_CRLF
static char lf[LWS_PRE + 1];
#endif

#ifdef DEBUG
#define debugf              printf
#else
#define debugf(...)         ((0))
#endif

#ifdef LOCAL_PRINT
#define local_printf        printf
#else
#define local_printf(...)   ((0))
#endif

static INLINE unsigned char *out_buffer_ptr(void) {
    return &ch[LWS_PRE];
}

static INLINE size_t out_buffer_len(void) {
    return chptr - LWS_PRE;
}

static INLINE bool out_buffer(unsigned char inch) {
    if (chptr < (LWS_PRE + OUT_BUFFER_SIZE)) {
        // only buffer if we have a socket...
        ch[chptr++] = inch;
        return true;
    } else {
        return false;
    }
}

static INLINE void reset_out_buffer(void) {
    chptr = LWS_PRE;
}

static int callback_serial(struct lws *wsi, enum lws_callback_reasons reason, UNUSED void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            if (wsi_global) {
                if (wsi_global == wsi) {
                    debugf("Establish again on existing wsi 🤔\n");
                } else {
                    debugf("Establish on new connection; Denying\n");
                }

                return -1;
            }
            debugf("WebSocket connection established\n");
            wsi_global = wsi;
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            lws_write(wsi, out_buffer_ptr(), out_buffer_len(), LWS_WRITE_BINARY);
            reset_out_buffer();
            break;

        case LWS_CALLBACK_RECEIVE:

            lws_write(wsi, in, len, LWS_WRITE_TEXT);

#ifdef INPUT_CR_TO_CRLF
            if (((char*)(in))[0] == 0xd) {
                printf("CR\n");
                lws_write(wsi, &lf[LWS_PRE], 1, LWS_WRITE_TEXT);
            }
#endif

            break;

        case LWS_CALLBACK_CLOSED:
            debugf("WebSocket connection closed\n");
            wsi_global = NULL;
            break;

        default:
            break;
    }

    return 0;
}

static int set_serial(int fd, int speed) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        fprintf(stderr, "Cannot tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Cannot tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static void cleanup(void) {
    if (from_ws_buf) {
        destroy_ring_buffer(from_ws_buf);
    }

    if (serial) {
        close(serial);
    }

    if (context) {
        lws_context_destroy(context);
    }
}

static bool opt_parse(int argc, char** argv) {
    int c;

    while ((c = getopt(argc, argv, ":b:d:")) != -1) {
        switch (c) {
            case 'b':
                baudrate = atoi(optarg);
                break;
            case 'd':
                device = optarg;
                break;
            case '?':
                if (optopt == 'b' || optopt == 'd') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint (optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }

                // fallthrough...

            default:
                return false;
        }
    }

    switch (baudrate) {
        case 9600:
            baudrate = B9600;
            break;
        case 115200:
            baudrate = B115200;
            break;
        default:
            fprintf(stderr, "Unsupported baudrate %d\n", baudrate);
            return false;
    }

    if (!device) {
        fprintf(stderr, "No device specified - use -d option!\n");
        return false;
    }

    return true;
}

static struct lws_protocols protocols[] = {
    {"serial-protocol", callback_serial, 0, 0, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0}};


int main(int argc, char** argv) {
    int result = 0;

    if (argc < 2) {
        printf("Usage: wsbbs <device>\n");
        return 100;
    }

    if (!opt_parse(argc, argv)) {
        return 255;
    }

    serial = open(device, O_RDWR | O_NOCTTY | O_SYNC);

    if (serial < 0) {
        fprintf(stderr, "Failed to open serial device '%s' (%s)\n", device, strerror(errno));
        result = 1;
        goto done;
    }

    if (set_serial(serial, baudrate) < 0) {
        fprintf(stderr, "Failed to setup serial device '%s' (%s)\n", device, strerror(errno));
        result = 2;
        goto done;
    }

    debugf("Serial device '%s' opened successfully...\n", device);

    reset_out_buffer();
    from_ws_buf = create_ring_buffer(256);

    if (from_ws_buf == NULL) {
        fprintf(stderr, "Failed to allocate ring buffers\n");
        result = 3;
        goto done;
    }

#ifdef INPUT_CR_TO_CRLF
    lf[LWS_PRE] = 0xa;
#endif
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));

    info.port = 8000;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);

    if (!context) {
        fprintf(stderr, "WebSocket context creation failed\n");
        result = 4;
        goto done;
    }

    printf("All systems go. Websocket listening on port 8000...\n");

#ifdef LOCAL_PRINT
    setvbuf(stdout, NULL, _IONBF, 0);
#endif

    fd_set rd;
    struct timeval tv = {0, 0};     // instant timeout to keep looping...
    int err;

    while (1) {
        // Service the websocket
        // TODO currently doing a hack with -1 timeout, really we should
        // be select()ing on both the socket and the serial below but need to
        // figure out how that works with libwebsocket (which uses poll() internally)
        // ...
        lws_service(context, -1);

        // Check on the serial connection...
        FD_ZERO(&rd);
        FD_SET(serial, &rd);

        err = select(serial+1, &rd, NULL, NULL, &tv);

        if (err == 0) {
            // timeout
            // just keep looping...
        } else if (err == -1) {
            // failure
            fprintf(stderr, "General failure on serial\n");
            result = 4;
            goto done;
        } else {
            // hoorah!
            unsigned char read_ch;
            
            if (read(serial, &read_ch, 1) != 1) {
                // read fail
                fprintf(stderr, "Read fail on serial\n");
                result = 5;
                goto done;
            } else {
                if (wsi_global) {
                    if (!out_buffer(read_ch)) {
                        fprintf(stderr, "Out buffer overflow; Websocket connection may be broken\n");
                        result = 6;
                        goto done;
                    }
                    
                    lws_callback_on_writable(wsi_global);
                    local_printf("\x1B[32m%c\x1B[0m", read_ch);
                } else {
                    local_printf("\x1B[31m%c\x1B[0m", read_ch);
                }
            }
        }
    }

done:
    cleanup();    
    return result;
}

