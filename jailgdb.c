// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// Jailhouse GDB stub communications program.

// Talks to the GDB stub on the bare metal cell using a custom Jailhouse
// hypercall that triggers an interrupt and a designated area of shared
// memory to exchange the data.

// GDB command: "target remote | jailgdb"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "fixed_addr.h"

struct port {
    uint8_t to_stub_avail;
    uint8_t to_stub_data;
    uint8_t from_stub_avail;
    uint8_t from_stub_data;
};

// XXX: We should include the Jailhouse headers here.
#define JAILHOUSE_DEBUG_INJECTIRQ      _IOW(0, 6, unsigned int)

#define JAILHOUSE_DEVICE        "/dev/jailhouse"
#define GDBSTUB_PORT_IRQ	125

static int open_dev()
{
    int fd;

    fd = open(JAILHOUSE_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("opening " JAILHOUSE_DEVICE);
        exit(1);
    }
    return fd;
}

static void jailhouse_irq(int fd)
{
    int err = ioctl(fd, JAILHOUSE_DEBUG_INJECTIRQ, GDBSTUB_PORT_IRQ);
    if (err) {
        perror("JAILHOUSE_DEBUG_INJECTIRQ");
        exit(1);
    }
}

static int port_avail(volatile struct port *port)
{
    return port->from_stub_avail != 0;
}

static void port_send(volatile struct port *port, char c)
{
    while (port->to_stub_avail) {
        usleep(100);
    }
    port->to_stub_data = c;
    port->to_stub_avail = 1;
}

static char port_read(volatile struct port *port)
{
    char data = port->from_stub_data;
    port->from_stub_avail = 0;
    return data;
}

int main(int argc, char **argv)
{
    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        perror("failed to open memory device");
        return 0;
    }

    volatile struct port *port = (volatile void *)mmap(NULL, 0x2000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd,
                                            GDBSTUB_PORT_ADDR);

    if (!port) {
        perror("failed to map debug port");
        return 0;
    }

    fcntl (0, F_SETFL, O_NONBLOCK);

    int jh = open_dev();

    int idle = 0;
    for (;;) {
        char c;
        idle++;

        int ret = read(0, &c, 1);
        if (ret < 0 && errno != EAGAIN)
                break;

        if (ret == 1) {
            port_send(port, c);
            jailhouse_irq(jh);
            idle = 0;
        }

        if (port_avail(port)) {
            char c = port_read(port);
            write(1, &c, 1);
            idle = 0;
        }

        if (idle > 10000) {
            usleep(1000);
        }
    }
}
