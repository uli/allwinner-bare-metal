// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// SDL event server
// Feeds SDL input events to a bare-metal program running in a Jailhouse cell.

#include "sdl_server.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_scancode.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

//#define DEBUG_PC
int debug = 0;

struct sdl_event_buffer *evbuf;

void printkey(SDL_KeyboardEvent kev)
{
    printf("k%s %d mod 0x%x @ %d\n", kev.type == SDL_KEYDOWN ? "down" : "up  ",
           kev.keysym.scancode, kev.keysym.mod, evbuf->write_pos);
}

int main(int argc, char **argv)
{
    SDL_Event event;

    setenv("SDL_VIDEODRIVER", "evdev", 1);

    if (argc > 1 && !strcmp(argv[1], "--debug"))
        debug = 1;

#ifdef DEBUG_PC

    // Create a dummy buffer for debugging without a receiver.
    evbuf = calloc(1, sizeof(*evbuf));

#else

    // Map the bare-metal cell's event ring buffer into our address space.

    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        perror("failed to open memory device");
        return 0;
    }

    evbuf = (struct sdl_event_buffer *)mmap(NULL, sizeof(*evbuf), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd,
            0x488f1000);

    if (!evbuf) {
        perror("failed to map event buffer");
        return 0;
    }

    memset(evbuf, 0, sizeof(*evbuf));

#endif

    if (SDL_Init(SDL_INIT_GAMECONTROLLER|SDL_INIT_VIDEO) < 0) {
        printf("failed to init SDL: %s\n", SDL_GetError());
        return 0;
    }

    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");

    for (;;) {
        SDL_WaitEvent(&event);

        if (debug) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    printkey(event.key);
                    // Quit on hotkey Ctrl+Ctrl+C.
                    if (event.key.keysym.scancode == SDL_SCANCODE_C &&
                        (event.key.keysym.mod & KMOD_LCTRL) &&
                        (event.key.keysym.mod & KMOD_RCTRL))
                        goto out;
                    break;
                case SDL_KEYUP:
                    printkey(event.key);
                    break;
                case SDL_QUIT:
                    goto out;
                default:
                    printf("type 0x%x event\n", event.type);
                    break;
            }
        }

        // For some reason, SDL2 sends a lot of events with (invalid) type
        // 0. We skip those.
        if (event.type != 0) {
            evbuf->events[evbuf->write_pos] = event;
            evbuf->write_pos = (evbuf->write_pos + 1) % SDL_EVENT_BUFFER_SIZE;
        }
    }

out:
    SDL_Quit();
    return 0;
}
