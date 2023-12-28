/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "prints.h"
#include "serial.h"
#include "opts.h"

bool opt_parse(int argc, char** argv, Options* opts) {
    int c;

    while ((c = getopt(argc, argv, ":b:d:")) != -1) {
        switch (c) {
            case 'b':
                *opts->baudrate = atoi(optarg);
                break;
            case 'd':
                *opts->device = optarg;
                break;
            case '?':
                if (optopt == 'b' || optopt == 'd') {
                    errorf("Option -%c requires an argument.\n", optopt);
                } else if (isprint (optopt)) {
                    errorf("Unknown option `-%c'.\n", optopt);
                } else {
                    errorf("Unknown option character `\\x%x'.\n", optopt);
                }

                // fallthrough...

            default:
                return false;
        }
    }

    if (speed_to_baudrate(*opts->baudrate) == 0) {
        errorf("Unsupported baudrate %d\n", *opts->baudrate);
        return false;
    }

    if (!*opts->device) {
        errorf("No device specified - use -d option!\n");
        return false;
    }

    return true;
}

