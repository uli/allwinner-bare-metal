// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// init comms

// Clears the inter-cell communication areas to prevent race conditions.

#include "libc_server.h"
#include "sdl_server.h"
#include "video_encoder.h"
#include "fixed_addr.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

void clear_comms_buffer(int mem_fd, unsigned long addr, size_t size)
{
    void *buffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, addr);
    if (buffer == MAP_FAILED) {
        perror("failed to map comms buffer");
        exit(1);
    }
    memset(buffer, 0, size);
    munmap(buffer, size);
}

int main(void)
{
    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        perror("failed to open memory device");
        return 1;
    }

    clear_comms_buffer(mem_fd, LIBC_CALL_BUFFER_ADDR, sizeof(struct libc_call_buffer));
    clear_comms_buffer(mem_fd, SDL_EVENT_BUFFER_ADDR, sizeof(struct sdl_event_buffer));
    clear_comms_buffer(mem_fd, VIDEO_ENCODER_PORT_ADDR, sizeof(struct video_encoder_comm_buffer));
    clear_comms_buffer(mem_fd, GDBSTUB_PORT_ADDR, 4);

    close(mem_fd);

    return 0;
}
