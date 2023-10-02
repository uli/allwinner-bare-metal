// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

void close_mic(void);
void open_mic(void);
int mix_mic(void);

extern int16_t *mix_buffer;

#define ALSA_AUDIO_DEVICE "default"
