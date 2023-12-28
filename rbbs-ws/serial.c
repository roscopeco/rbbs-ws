/*
 * rbbs-ws 
 *
 * Copyright (c)2023 Ross Bamford & Contributors
 * MIT License
 */

#include <errno.h>
#include <string.h>

#include "prints.h"
#include "serial.h"

speed_t speed_to_baudrate(int speed) {
    switch (speed) {
    case 9600:
        return B9600;
    case 115200:
        return B115200;
    default:
        return 0;
    }
}

int set_serial(int fd, int speed) {
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        errorf("Cannot tcgetattr: %s\n", strerror(errno));
        return 1;
    }

    int baudrate = speed_to_baudrate(speed);
    if (baudrate == 0) {
        errorf("Unsupported baudrate %d\n", baudrate);
        return 2;
    }

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

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
        errorf("Cannot tcsetattr: %s\n", strerror(errno));
        return 3;
    }
    return 0;
}

