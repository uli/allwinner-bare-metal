// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// Records input from an ALSA capture device and mixes it with video
// recorder audio.

#include <alsa/asoundlib.h>
#include "audio_in.h"
#include "../video_encoder.h"

snd_pcm_t *capture_handle = NULL;
int16_t *mic_buffer = NULL;
int16_t *mix_buffer = NULL;

void close_mic(void)
{
	printf("closing alsa\n");

	if (capture_handle) {
		snd_pcm_close(capture_handle);
		capture_handle = NULL;
	}

	free(mic_buffer);
	mic_buffer = NULL;

	free(mix_buffer);
	mix_buffer = NULL;
}

// adapted from http://equalarea.com/paul/alsa-audio.html
void open_mic(void)
{
	int err;
	unsigned int rate = VE_AUDIO_SAMPLE_RATE;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

	if ((err = snd_pcm_open(&capture_handle, ALSA_AUDIO_DEVICE, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
		printf("cannot open ALSA audio device %s (%s)\n",
			ALSA_AUDIO_DEVICE, snd_strerror(err));
		return;
	}

	printf("ALSA audio interface opened\n");

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		printf("cannot allocate hardware parameter structure (%s)\n",
			snd_strerror(err));
		goto out;
	}

	if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
		printf("cannot initialize hardware parameter structure (%s)\n",
			snd_strerror(err));
		goto out;
	}

	if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		printf("cannot set access type (%s)\n", snd_strerror(err));
		goto out;
	}

	if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
		printf("cannot set sample format (%s)\n", snd_strerror(err));
		goto out;
	}

	if ((err = snd_pcm_hw_params_set_rate(capture_handle, hw_params, rate, 0)) < 0) {
		printf("cannot set sample rate (%s)\n", snd_strerror(err));
		goto out;
	}

	if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, VE_AUDIO_CHANNELS)) < 0) {
		printf("cannot set channel count (%s)\n", snd_strerror(err));
		goto out;
	}

	if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
		printf("cannot set parameters (%s)\n", snd_strerror(err));
		goto out;
	}

	snd_pcm_hw_params_free(hw_params);

	if ((err = snd_pcm_prepare(capture_handle)) < 0) {
		printf("cannot prepare audio interface for use (%s)\n", snd_strerror(err));
		goto out;
	}

	mic_buffer = malloc(video_encoder->audio_size / VE_AUDIO_CHANNELS);	// one channel
	mix_buffer = malloc(video_encoder->audio_size);				// all channels

	if (!mic_buffer || !mix_buffer) {
		printf("could not allocate audio input buffers\n");
		goto out;
	}

	if ((err = snd_pcm_start(capture_handle)) < 0) {
		printf("could not start PCM capture: %s\n", snd_strerror(err));
		goto out;
	}

	return;

out:
	close_mic();
}

int mix_mic(void)
{
	long int alsa_asize = video_encoder->audio_size / VE_AUDIO_SAMPLE_BYTES / VE_AUDIO_CHANNELS;

	if (capture_handle && snd_pcm_avail(capture_handle) >= alsa_asize) {
		if (snd_pcm_readi(capture_handle, mic_buffer, alsa_asize) == alsa_asize) {
			int16_t *abuf = video_encoder->audio_buffer;
			for (int i = 0; i < alsa_asize; ++i) {
				// XXX: only works for two channels
				mix_buffer[i * VE_AUDIO_CHANNELS]     = (abuf[i * VE_AUDIO_CHANNELS]     + mic_buffer[i]) / 2;
				mix_buffer[i * VE_AUDIO_CHANNELS + 1] = (abuf[i * VE_AUDIO_CHANNELS + 1] + mic_buffer[i]) / 2;
			}
			return alsa_asize;
		}
	}
	return 0;
}
