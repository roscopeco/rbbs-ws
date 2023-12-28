/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#ifndef __RBBS_WS_OPTS_H
#define __RBBS_WS_OPTS_H

typedef struct  {
    int *baudrate;
    char **device;
} Options;

bool opt_parse(int argc, char** argv, Options* opts);

#endif  // _RBBS_WS_OPTS_H


