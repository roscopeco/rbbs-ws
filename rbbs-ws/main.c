/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libwebsockets.h>

#include "config.h"
#include "ring_buffer.h"
#include "out_buffer.h"
#include "serial.h"
#include "opts.h"
#include "prints.h"

static struct lws *wsi_global;
static int serial;
static RingBuffer *from_ws_buf;
static int baudrate = 115200;
static char* device;
static unsigned char busy_buf[LWS_PRE + 6];
static struct lws_context *context;

#ifdef INPUT_CR_TO_CRLF
static char lf[LWS_PRE + 1];
#endif

static int callback_serial(struct lws *wsi, enum lws_callback_reasons reason, UNUSED void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            if (wsi_global) {
                if (wsi_global == wsi) {
                    debugf("Establish again on existing wsi 🤔\n");
                } else {
                    debugf("Establish on new connection; Denying\n");
                }

                lws_callback_on_writable(wsi);

                break;
            }

            debugf("WebSocket connection established\n");
            wsi_global = wsi;
            break;

        case LWS_CALLBACK_SERVER_WRITEABLE:
            if (wsi_global) {
                if (wsi != wsi_global) {
                    lws_write(wsi, &busy_buf[LWS_PRE], BUSY_LEN, LWS_WRITE_BINARY);
                    return -1;
                }

                lws_write(wsi, out_buffer_ptr(), out_buffer_len(), LWS_WRITE_BINARY);
                reset_out_buffer();
            }

            break;

        case LWS_CALLBACK_RECEIVE:
            // TODO this should be buffered and check we can write first!
            write(serial, in, len);

#ifdef INPUT_CR_TO_CRLF
            if (((char*)(in))[0] == 0xd) {
                write(serial, &lf[LWS_PRE], 1);
            }
#endif

            break;

        case LWS_CALLBACK_CLOSED:
            if (wsi == wsi_global) {
                debugf("Main WebSocket connection closed\n");
                wsi_global = NULL;
                reset_out_buffer();
            } else {
                debugf("Busy WebSocket connection closed\n");
            }

            break;

        default:
            break;
    }

    return 0;
}

static struct lws_protocols protocols[] = {
    {"serial-protocol", callback_serial, 0, 0, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0}};

static INLINE void cleanup(void) {
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

static bool parse_options(int argc, char** argv) {
    Options opts;
    opts.baudrate = &baudrate;
    opts.device = &device;

    return opt_parse(argc, argv, &opts);
}

int main(int argc, char** argv) {
    memcpy(&busy_buf[LWS_PRE], BUSY_STR, BUSY_LEN);

    int result = 0;

    if (argc < 2) {
        printf("Usage: wsbbs <device>\n");
        return 100;
    }

    if (!parse_options(argc, argv)) {
        return 255;
    }

    serial = open(device, O_RDWR | O_NOCTTY | O_SYNC);

    if (serial < 0) {
        errorf("Failed to open serial device '%s' (%s)\n", device, strerror(errno));
        result = 1;
        goto done;
    }

    if (set_serial(serial, baudrate) < 0) {
        errorf("Failed to setup serial device '%s' (%s)\n", device, strerror(errno));
        result = 2;
        goto done;
    }

    debugf("Serial device '%s' opened successfully...\n", device);

    reset_out_buffer();
    from_ws_buf = create_ring_buffer(256);

    if (from_ws_buf == NULL) {
        errorf("Failed to allocate ring buffers\n");
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
                        errorf("Out buffer overflow; Websocket connection may be broken\n");
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

