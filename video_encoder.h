// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// Interface to H.264 encoder process on the Linux side.

#include "fixed_addr.h"

#define VE_AUDIO_SAMPLE_RATE 48000
#define VE_AUDIO_SAMPLE_BYTES 2	// 16 bit
#define VE_AUDIO_CHANNELS 2

struct video_encoder_comm_buffer {
    void *video_luma;
    void *video_chroma;
    int video_frame_no;
    int video_w;
    int video_h;
    void *audio_buffer;
    int audio_frame_no;
    int audio_size;
    int enabled;
};

static volatile struct video_encoder_comm_buffer *video_encoder __attribute__((unused)) =
    (volatile struct video_encoder_comm_buffer *)VIDEO_ENCODER_PORT_ADDR;
