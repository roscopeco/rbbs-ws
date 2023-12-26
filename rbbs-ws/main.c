#include <libwebsockets.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

#define LOCAL_PRINT
#define DEBUG
//#define INPUT_CR_TO_CRLF

static struct lws *wsi_global;
static unsigned char buf[LWS_PRE + 40];
static unsigned char ch[LWS_PRE + 1];
int serial;

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

static int callback_serial(struct lws *wsi, enum lws_callback_reasons reason,
        void *user, void *in, size_t len) {
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
            lws_write(wsi, &ch[LWS_PRE], 1, LWS_WRITE_TEXT);
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

static void cleanup() {
    if (serial) {
        close(serial);
    }

    if (context) {
        lws_context_destroy(context);
    }
}

static struct lws_protocols protocols[] = {
    {"serial-protocol", callback_serial, 0, 0},
    {NULL, NULL, 0, 0}};

int main(int argc, char** argv) {
    int result = 0;

    if (argc < 2) {
        printf("Usage: wsbbs <device>\n");
        return 100;
    }

    serial = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC);

    if (serial < 0) {
        fprintf(stderr, "Failed to open serial device '%s'\n", argv[1]);
        result = 1;
        goto done;
    }

    if (set_serial(serial, B115200) < 0) {
        fprintf(stderr, "Failed to setup serial device '%s'\n", argv[1]);
        result = 2;
        goto done;
    }

    debugf("Serial device '%s' opened successfully...\n", argv[1]);

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
        result = 3;
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
            if (read(serial, &ch[LWS_PRE], 1) != 1) {
                // read fail
                fprintf(stderr, "Read fail on serial\n");
                result = 5;
                goto done;
            } else {
                if (wsi_global) { 
                    lws_callback_on_writable(wsi_global);
                    local_printf("\x1B[31m%c\x1B[0m", ch[LWS_PRE]);
                } else {
                    local_printf("\x1B[32m%c\x1B[0m", ch[LWS_PRE]);
                }
            }
        }
    }

done:
    cleanup();    
    return result;
}

