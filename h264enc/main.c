/*
 * Copyright (c) 2014 Jens Kuske <jenskuske@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "ve.h"
#include "h264enc.h"
#include "h264avi.h"

// Engine BASIC interface
#include "../video_encoder.h"

int last_video_frame_no = 0;
int last_audio_frame_no = 0;
void h264enc_set_buffers(h264enc * c, uint8_t *luma, uint8_t *chroma);

int quit = 0;

void sigint_handler(int dummy)
{
    (void)dummy;
    quit = 1;
}

int main(const int argc, const char **argv)
{
    (void)argc; (void)argv;

    signal(SIGINT, sigint_handler);

    // Map shared memory communication regions.
    // We use fixed ID mappings so we don't have to translate bare-metal pointers.

    int mem_fd = open("/dev/mem", O_RDWR);
    if (mem_fd < 0) {
        perror("failed to open memory device");
        return 0;
    }

    // Map BASIC program memory (text, data, bss)
    // XXX: We would like to map up to 2GB here, but that fails for lack of
    // available address space, so we leave 256MB unused on 2GB devices.
    void *basic_mem = mmap((void *)AWBM_BASE_ADDR, 0x67000000, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, AWBM_BASE_ADDR);
    if (basic_mem == MAP_FAILED) {
        perror("failed to map BASIC memory");
        return 0;
    }
    printf("BASIC mem at %p\n", basic_mem);

    // Map communications area.
    video_encoder = (struct video_encoder_comm_buffer *)mmap(
    	(void *)VIDEO_ENCODER_PORT_ADDR,
	sizeof(*video_encoder),
	PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
	mem_fd, VIDEO_ENCODER_PORT_ADDR);

    if (video_encoder == MAP_FAILED) {
        perror("failed to map comms area");
        return 0;
    }

    int file_count = 0;
    char out_file_name[] = "/sd/rec0000.avi";

    int width;
    int height;

    while(!quit) {
    	if (!video_encoder->enabled) {
    		usleep(100000);
    		continue;
	}

	width = video_encoder->video_w;
	height = video_encoder->video_h;

	if (!ve_open())
		return EXIT_FAILURE;

	// Find lowest unused file name.
	while (file_count < 10000) {
		struct stat st;

		sprintf(out_file_name, "/sd/rec%04d.avi", file_count);

		if (stat(out_file_name, &st))
			break;

		++file_count;
	}

	FILE *out;

	if ((out = fopen(out_file_name, "w+")) == NULL)
	{
		printf("could not open output file\n");
		return EXIT_FAILURE;
	}

	struct avi_context *avi = avi_new_file(out);
	avi_write_header(avi, width, height, 30);

	struct h264enc_params params;
	params.src_width = (width + 15) & ~15;
	params.width = width;
	params.src_height = (height + 15) & ~15;
	params.height = height;
	params.src_format = H264_FMT_NV12;
	params.profile_idc = 77;
	params.level_idc = 41;
	params.entropy_coding_mode = H264_EC_CABAC;
	params.qp = 24;
	params.keyframe_interval = 25;

	h264enc *encoder = h264enc_new(&params);
	if (encoder == NULL)
	{
		printf("could not create encoder\n");
		goto err;
	}

	void* output_buf = h264enc_get_bytestream_buffer(encoder);

	int input_size = params.src_width * (params.src_height + params.src_height / 2);
	void* input_buf = h264enc_get_input_buffer(encoder);

	printf("Running h264 encoding");

	while (video_encoder->enabled && !quit) {
		while (video_encoder->video_frame_no == last_video_frame_no &&
		       video_encoder->audio_frame_no == last_audio_frame_no) {
			asm("wfe");
		}
		if (video_encoder->audio_frame_no != last_audio_frame_no) {
			if (video_encoder->audio_frame_no > last_audio_frame_no + 1)
				printf("audio frame drop %d->%d\n", last_audio_frame_no, video_encoder->audio_frame_no);

			last_audio_frame_no = video_encoder->audio_frame_no;
			avi_write_audio(avi, video_encoder->audio_buffer, video_encoder->audio_size);
		}
		if (video_encoder->video_frame_no != last_video_frame_no) {

			if (video_encoder->video_frame_no > last_video_frame_no + 2)
				printf("video frame drop %d->%d\n", last_video_frame_no, video_encoder->video_frame_no);

			last_video_frame_no = video_encoder->video_frame_no;
			h264enc_set_buffers(encoder, video_encoder->video_luma, video_encoder->video_chroma);
			if (h264enc_encode_picture(encoder)) {
				avi_write_video(avi, output_buf, h264enc_get_bytestream_length(encoder));
			} else {
				printf("encoding error\n");
			}
		}
	}

	printf("Done!\n");

	h264enc_free(encoder);

err:
	ve_close();
	avi_finalize(avi);
	fclose(out);
    }

    return EXIT_SUCCESS;
}
