#ifndef __WAV_PRIV_H__
#define __WAV_PRIV_H__

#include <stdint.h>
#include <stdio.h>

#include "wav_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__x86_64) || defined(__amd64) || defined(__i386__) || defined(__x86_64__) || defined(__LITTLE_ENDIAN__)
#define WAV_ENDIAN_LITTLE 1
#elif defined(__BIG_ENDIAN__)
#define WAV_ENDIAN_BIG 1
#endif

#if WAV_ENDIAN_LITTLE
#define WAV_RIFF_CHUNK_ID 'FFIR'
#define WAV_FORMAT_CHUNK_ID ' tmf'
#define WAV_FACT_CHUNK_ID 'tcaf'
#define WAV_DATA_CHUNK_ID 'atad'
#define WAV_WAVE_ID 'EVAW'
#endif

#if WAV_ENDIAN_BIG
#define WAV_RIFF_CHUNK_ID 'RIFF'
#define WAV_FORMAT_CHUNK_ID 'fmt '
#define WAV_FACT_CHUNK_ID 'fact'
#define WAV_DATA_CHUNK_ID 'data'
#define WAV_WAVE_ID 'WAVE'
#endif

#pragma pack(push)
#pragma pack(1)

#define WAV_RIFF_HEADER_SIZE 8

typedef struct _WavFormatChunk WavFormatChunk;
struct _WavFormatChunk {
    /* RIFF header */
    uint32_t id;
    uint32_t size;

    uint16_t format_tag;
    uint16_t n_channels;
    uint32_t sample_rate;
    uint32_t avg_bytes_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;

    uint16_t ext_size;
    uint16_t valid_bits_per_sample;
    uint32_t channel_mask;

    uint8_t sub_format[16];
};

typedef struct _WavFactChunk WavFactChunk;
struct _WavFactChunk {
    /* RIFF header */
    uint32_t id;
    uint32_t size;

    uint32_t sample_length;
};

typedef struct _WavDataChunk WavDataChunk;
struct _WavDataChunk {
    /* RIFF header */
    uint32_t id;
    uint32_t size;
};

typedef struct _WavMasterChunk WavMasterChunk;
struct _WavMasterChunk {
    /* RIFF header */
    uint32_t id;
    uint32_t size;

    uint32_t wave_id;

    WavFormatChunk format_chunk;
    WavFactChunk   fact_chunk;
    WavDataChunk   data_chunk;
};

#pragma pack(pop)

struct _WavFile {
    FILE*          fp;
    char*          mode;
    WavError       error_code;
    WavMasterChunk chunk;
    uint8_t*       tmp;
    size_t         tmp_size;
};

#ifdef __cplusplus
}
#endif

#endif /* __WAV_PRIV_H__ */
