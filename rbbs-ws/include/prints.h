/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#ifndef __RBBS_WS_PRINTS_H
#define __RBBS_WS_PRINTS_H

#include <stdio.h>

#define errorf(...)         fprintf(stderr, __VA_ARGS__)

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

#endif//__RBBS_WS_PRINTS_H
