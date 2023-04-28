// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// SDL event server data structures

// WARNING: The allwinner-bare-metal toolchain as compiled with the included
// crosstool-ng configuration file defaults to compile code with
// "-fshort-enums", while Linux toolchains use "-fno-short-enums".

// Since SDL2 data structures make extensive use of enums, and the data is
// produced on the Linux side, it is mandatory to compile any bare-metal
// code that uses these data structures with "-fno-short-enums". The SDL
// header files check enum sizes and will throw an error if the size is not
// the same as an int.

// You will also have to make sure that any code compiled with
// "-fno-short-enums" doesn't use any bare-metal data structures with
// enums...

// There doesn't seem to be any easy way to configure the bare-metal
// toolchain to use "-fno-short-enums", unfortunately.

#include <SDL2/SDL_events.h>

#define SDL_EVENT_BUFFER_SIZE 128

struct sdl_event_buffer {
    int read_pos;
    int write_pos;
    SDL_Event events[SDL_EVENT_BUFFER_SIZE];
};
