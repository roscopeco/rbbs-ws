/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#ifndef __RBBS_WS_SERIAL_H
#define __RBBS_WS_SERIAL_H

#include <termios.h>

speed_t speed_to_baudrate(int speed);

int set_serial(int fd, int speed);

#endif  // _RBBS_WS_SERIAL_H
