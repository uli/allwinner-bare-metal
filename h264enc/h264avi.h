// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

#include <stdio.h>

struct avi_context;

struct avi_context *avi_new_file(FILE *fp);
void avi_write_header(struct avi_context *c, int w, int h, int fps);
void avi_write_video(struct avi_context *c, void *frame, int size);
void avi_write_audio(struct avi_context *c, void *samples, int size);
void avi_finalize(struct avi_context *c);
