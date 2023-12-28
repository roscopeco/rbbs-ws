/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#include <stdlib.h>
#include <stdbool.h>
#include <libwebsockets.h>

#include "out_buffer.h"

// UART to WS buffer doesn't use a RingBuffer, since we need to keep the 
// LWS_PRE preamble for the websocket write anyway we may as well just 
// keep a buffer after that. We don't bother treating it as a ring buffer,
// if we can't send within the time it takes to fill, something is wrong anyhow...
static unsigned char ch[LWS_PRE + OUT_BUFFER_SIZE];
static int chptr;

unsigned char *out_buffer_ptr(void) {
    return &ch[LWS_PRE];
}

size_t out_buffer_len(void) {
    return chptr - LWS_PRE;
}

bool out_buffer(unsigned char inch) {
    if (chptr < (LWS_PRE + OUT_BUFFER_SIZE)) {
        // only buffer if we have room...
        ch[chptr++] = inch;
        return true;
    } else {
        return false;
    }
}

void reset_out_buffer(void) {
    chptr = LWS_PRE;
}
