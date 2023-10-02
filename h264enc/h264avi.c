// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

// Creates AVI files with H.264 video and PCM audio track.

#include "h264avi.h"
#include "../video_encoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int FOURCC;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned short WORD;

typedef struct _avimainheader {
  FOURCC fcc;
  DWORD  cb;
  DWORD  dwMicroSecPerFrame;
  DWORD  dwMaxBytesPerSec;
  DWORD  dwPaddingGranularity;
  DWORD  dwFlags;
  DWORD  dwTotalFrames;
  DWORD  dwInitialFrames;
  DWORD  dwStreams;
  DWORD  dwSuggestedBufferSize;
  DWORD  dwWidth;
  DWORD  dwHeight;
  DWORD  dwReserved[4];
} AVIMAINHEADER;

typedef struct {
  WORD   left;
  WORD   top;
  WORD   right;
  WORD   bottom;
} RECT;

typedef struct {
  FOURCC fccType;
  FOURCC fccHandler;
  DWORD  dwFlags;
  WORD   wPriority;
  WORD   wLanguage;
  DWORD  dwInitialFrames;
  DWORD  dwScale;
  DWORD  dwRate;
  DWORD  dwStart;
  DWORD  dwLength;
  DWORD  dwSuggestedBufferSize;
  DWORD  dwQuality;
  DWORD  dwSampleSize;
  RECT   rcFrame;
} AVIStreamHeader;

typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct {
  WORD  wFormatTag;
  WORD  nChannels;
  DWORD nSamplesPerSec;
  DWORD nAvgBytesPerSec;
  WORD  nBlockAlign;
  WORD  wBitsPerSample;
  WORD  cbSize;
} WAVEFORMATEX;

typedef struct {
  DWORD dwChunkId;
  DWORD dwFlags;
  DWORD dwOffset;
  DWORD dwSize;
} _avioldindex_entry;

typedef struct _avioldindex {
  FOURCC             fcc;
  DWORD              cb;
  _avioldindex_entry aIndex[];
} AVIOLDINDEX;

struct avi_context {
    size_t hdrl_list_size_pos;
    size_t hdrl_pos;
    size_t strl_auds_list_size_pos;
    size_t strl_auds_pos;
    size_t strl_vids_list_size_pos;
    size_t strl_vids_pos;

    size_t movi_list_size_pos;

    AVIMAINHEADER hdrl;
    AVIStreamHeader vids;
    BITMAPINFOHEADER strfv;
    AVIStreamHeader auds;
    WAVEFORMATEX strfa;

    int frame_count;
    int sample_count;

    FILE *fp;
};

struct avi_context *avi_new_file(FILE *fp)
{
    struct avi_context *c = calloc(1, sizeof(struct avi_context));
    c->fp = fp;
    return c;
}

void avi_write_header(struct avi_context *c, int w, int h, int fps)
{
    FILE *fp = c->fp;

    fprintf(fp, "RIFF....AVI LIST");
    c->hdrl_list_size_pos = ftell(fp);

    fprintf(fp, "....hdrl");
    c->hdrl_pos = ftell(fp);

    memset(&c->hdrl, 0, sizeof(c->hdrl));
    c->hdrl.fcc = 0x68697661; // "avih"
    c->hdrl.cb = sizeof(AVIMAINHEADER) - 8;
    c->hdrl.dwMicroSecPerFrame = 1000000 / fps;
    c->hdrl.dwMaxBytesPerSec = 192000; // ???
    c->hdrl.dwPaddingGranularity = 0;
    c->hdrl.dwFlags = 2320; // ???
    // fill in hdrl.dwTotalFrames in avi_finalize()
    c->hdrl.dwInitialFrames = 0;
    c->hdrl.dwStreams = 2;
    c->hdrl.dwSuggestedBufferSize = 1048576; // ???
    c->hdrl.dwWidth = w;
    c->hdrl.dwHeight = h;

    fwrite(&c->hdrl, sizeof(c->hdrl), 1, fp);

    fprintf(fp, "LIST");
    c->strl_vids_list_size_pos = ftell(fp);

    fprintf(fp, "....strlstrh");
    unsigned int size = sizeof(AVIStreamHeader);
    fwrite(&size, 4, 1, fp);
    c->strl_vids_pos = ftell(fp);

    memset(&c->vids, 0, sizeof(c->vids));
    c->vids.fccType = 0x73646976; // "vids"
    c->vids.fccHandler = 0x34363248; // "H264"
    c->vids.dwFlags = 0;
    c->vids.wPriority = 0;
    c->vids.wLanguage = 0;
    c->vids.dwInitialFrames = 0;
    c->vids.dwScale = 1;
    c->vids.dwRate = fps;
    c->vids.dwStart = 0;
    // fill in c->vids.dwLength (frames) in avi_finalize()
    c->vids.dwSuggestedBufferSize = 8066; // ???
    c->vids.dwQuality = 4294967295; // ???
    c->vids.dwSampleSize = 0;
    c->vids.rcFrame.left = 0;
    c->vids.rcFrame.top = 0;
    c->vids.rcFrame.right = w;
    c->vids.rcFrame.bottom = h;
    fwrite(&c->vids, sizeof(c->vids), 1, fp);

    fprintf(fp, "strf");
    size = sizeof(BITMAPINFOHEADER);
    fwrite(&size, 4, 1, fp);

    memset(&c->strfv, 0, sizeof(c->strfv));
    c->strfv.biSize = sizeof(c->strfv);
    c->strfv.biWidth = w;
    c->strfv.biHeight = h;
    c->strfv.biPlanes = 1;
    c->strfv.biBitCount = 24;
    c->strfv.biCompression = 0x34363248; // "H264"
    c->strfv.biSizeImage = w * h * 3;
    fwrite(&c->strfv, sizeof(c->strfv), 1, fp);

    size_t end_of_vids_list = ftell(fp);
    unsigned int vids_list_size = end_of_vids_list - c->strl_vids_list_size_pos - 4;
    fseek(fp, c->strl_vids_list_size_pos, SEEK_SET);
    fwrite(&vids_list_size, 4, 1, fp);
    fseek(fp, end_of_vids_list, SEEK_SET);

    fprintf(fp, "LIST");
    c->strl_auds_list_size_pos = ftell(fp);

    fprintf(fp, "....strlstrh");
    size = sizeof(AVIStreamHeader);
    fwrite(&size, 4, 1, fp);
    c->strl_auds_pos = ftell(fp);

    memset(&c->auds, 0, sizeof(c->auds));
    c->auds.fccType = 0x73647561; // "auds"
    c->auds.fccHandler = 0x00000001;
    c->auds.dwFlags = 0;
    c->auds.wPriority = 0;
    c->auds.wLanguage = 0;
    c->auds.dwInitialFrames = 0;
    c->auds.dwScale = 1;
    c->auds.dwRate = VE_AUDIO_SAMPLE_RATE;
    c->auds.dwStart = 0;
    // fill in c->auds.dwLength (samples) in avi_finalize()
    c->auds.dwSuggestedBufferSize = 4096; // ???
    c->auds.dwQuality = 4294967295; // ???
    c->auds.dwSampleSize = 4;
    c->auds.rcFrame.left = 0;
    c->auds.rcFrame.top = 0;
    c->auds.rcFrame.right = 0;
    c->auds.rcFrame.bottom = 0;
    fwrite(&c->auds, sizeof(c->auds), 1, fp);

    fprintf(fp, "strf");
    size = sizeof(WAVEFORMATEX);
    fwrite(&size, 4, 1, fp);

    memset(&c->strfa, 0, sizeof(c->strfa));
    c->strfa.wFormatTag = 1;
    c->strfa.nChannels = VE_AUDIO_CHANNELS;
    c->strfa.nSamplesPerSec = VE_AUDIO_SAMPLE_RATE;
    c->strfa.nAvgBytesPerSec = VE_AUDIO_SAMPLE_RATE * VE_AUDIO_CHANNELS * VE_AUDIO_SAMPLE_BYTES;
    c->strfa.nBlockAlign = VE_AUDIO_CHANNELS * VE_AUDIO_SAMPLE_BYTES;
    c->strfa.wBitsPerSample = VE_AUDIO_SAMPLE_BYTES * 8;
    c->strfa.cbSize = 0; // ???
    fwrite(&c->strfa, sizeof(c->strfa), 1, fp);

    size_t data_list_pos = ftell(fp);

    unsigned int hdrl_list_size = data_list_pos - c->hdrl_list_size_pos - 4;
    fseek(fp, c->hdrl_list_size_pos, SEEK_SET);
    fwrite(&hdrl_list_size, 4, 1, fp);

    unsigned int auds_list_size = data_list_pos - c->strl_auds_list_size_pos - 4;
    fseek(fp, c->strl_auds_list_size_pos, SEEK_SET);
    fwrite(&auds_list_size, 4, 1, fp);

    fseek(fp, data_list_pos, SEEK_SET);

    fprintf(fp, "LIST");
    c->movi_list_size_pos = ftell(fp);
    fprintf(fp, "....movi");
}

void avi_write_video(struct avi_context *c, void *frame, int size)
{
    // XXX: index?
    fprintf(c->fp, "00dc");
    fwrite(&size, 4, 1, c->fp);
    fwrite(frame, size, 1, c->fp);

    // Looks like chunks in movi need to be aligned to 16 bits.
    // FFMPEG does it, players expect it, yet nobody seems to ever mention
    // it anywhere...
    while (ftell(c->fp) % 2 != 0)
      fputc(0, c->fp);

    ++c->frame_count;
}

void avi_write_audio(struct avi_context *c, void *samples, int size)
{
    // XXX: index?
    fprintf(c->fp, "01wb");
    fwrite(&size, 4, 1, c->fp);
    fwrite(samples, size, 1, c->fp);
    while (ftell(c->fp) % 2 != 0)
      fputc(0, c->fp);
    c->sample_count += size / VE_AUDIO_CHANNELS / VE_AUDIO_SAMPLE_BYTES;
}

void avi_finalize(struct avi_context *c)
{
    FILE *fp = c->fp;

    // XXX: index?

    long end_of_movi = ftell(fp);

    c->hdrl.dwTotalFrames = c->frame_count;
    fseek(fp, c->hdrl_pos, SEEK_SET);
    fwrite(&c->hdrl, sizeof(c->hdrl), 1, fp);

    c->vids.dwLength = c->frame_count;
    fseek(fp, c->strl_vids_pos, SEEK_SET);
    fwrite(&c->vids, sizeof(c->vids), 1, fp);

    c->auds.dwLength = c->sample_count;
    fseek(fp, c->strl_auds_pos, SEEK_SET);
    fwrite(&c->auds, sizeof(c->auds), 1, fp);

    fseek(fp, c->movi_list_size_pos, SEEK_SET);
    unsigned int movi_size = end_of_movi - c->movi_list_size_pos - 4;
    fwrite(&movi_size, 4, 1, fp);

    fseek(fp, 0, SEEK_END);

    fprintf(fp, "idx1");
    unsigned int index_size_pos = ftell(fp);
    fprintf(fp, "....");

    // Build index.

    // go to first chunk in "movi"
    fseek(fp, c->movi_list_size_pos + 8, SEEK_SET);

    while (ftell(fp) < end_of_movi) {
        unsigned int chunk_id;
        unsigned int chunk_size;

        unsigned int current_chunk_pos = ftell(fp);
        fread(&chunk_id, 4, 1, fp);
        fread(&chunk_size, 4, 1, fp);

        fseek(fp, 0, SEEK_END);

        _avioldindex_entry entry = {
          .dwChunkId = chunk_id,
          .dwFlags = 0x10,	// keyframe?
          .dwOffset = current_chunk_pos - c->movi_list_size_pos - 4,
          .dwSize = chunk_size,
        };

        fwrite(&entry, sizeof(entry), 1, fp);

        chunk_size = (chunk_size + 1) & ~1;	// remember the alignment

        // go to next chunk
        fseek(fp, current_chunk_pos + chunk_size + 8, SEEK_SET);
    }

    fseek(fp, 0, SEEK_END);

    unsigned int total_size = ftell(fp);
    unsigned int total_size_for_riff_hdr = total_size - 8;
    unsigned int index_size = total_size - index_size_pos - 4;

    fseek(fp, index_size_pos, SEEK_SET);
    fwrite(&index_size, 4, 1, fp);

    fseek(fp, 4, SEEK_SET);
    fwrite(&total_size_for_riff_hdr, 4, 1, fp);

    fseek(fp, 0, SEEK_END);
}
