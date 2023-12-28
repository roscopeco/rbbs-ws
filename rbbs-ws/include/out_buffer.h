/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#ifndef __RBBS_WS_OUT_BUFFER_H
#define __RBBS_WS_OUT_BUFFER_H

#include <stdlib.h>

#define OUT_BUFFER_SIZE     256

unsigned char *out_buffer_ptr(void);
size_t out_buffer_len(void);
bool out_buffer(unsigned char inch);
void reset_out_buffer(void);

#endif  // _RBBS_WS_OUT_BUFFER_H


