#ifndef __WAVE_PRIV_H__
#define __WAVE_PRIV_H__

#include <stdio.h>

#include "wave_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define WAVE_ENDIAN_LITTLE
#endif

#ifdef WAVE_ENDIAN_LITTLE
#define WAVE_RIFF_CHUNK_ID          'FFIR'
#define WAVE_FORMAT_CHUNK_ID        ' tmf'
#define WAVE_FACT_CHUNK_ID          'tcaf'
#define WAVE_DATA_CHUNK_ID          'atad'
#define WAVE_WAVE_ID                'EVAW'
#endif

#ifdef WAVE_ENDIAN_BIG
#define WAVE_RIFF_CHUNK_ID          'RIFF'
#define WAVE_FORMAT_CHUNK_ID        'fmt '
#define WAVE_FACT_CHUNK_ID          'fact'
#define WAVE_DATA_CHUNK_ID          'data'
#define WAVE_WAVE_ID                'WAVE'
#endif

#pragma pack(push)
#pragma pack(1)

#define WAVE_RIFF_HEADER_SIZE   8

typedef struct _WaveFormatChunk WaveFormatChunk;
struct _WaveFormatChunk {
    /* RIFF header */
    WaveUInt32  id;
    WaveUInt32  size;

    WaveUInt16  format_tag;
    WaveUInt16  n_channels;
    WaveUInt32  sample_rate;
    WaveUInt32  avg_bytes_per_sec;
    WaveUInt16  block_align;
    WaveUInt16  bits_per_sample;

    WaveUInt16  ext_size;
    WaveUInt16  valid_bits_per_sample;
    WaveUInt32  channel_mask;

    WaveUInt8   sub_format[16];
};

typedef struct _WaveFactChunk WaveFactChunk;
struct _WaveFactChunk {
    /* RIFF header */
    WaveUInt32  id;
    WaveUInt32  size;

    WaveUInt32  sample_length;
};

typedef struct _WaveDataChunk WaveDataChunk;
struct _WaveDataChunk {
    /* RIFF header */
    WaveUInt32  id;
    WaveUInt32  size;
};

typedef struct _WaveMasterChunk WaveMasterChunk;
struct _WaveMasterChunk {
    /* RIFF header */
    WaveUInt32          id;
    WaveUInt32          size;

    WaveUInt32          wave_id;

    WaveFormatChunk     format_chunk;
    WaveFactChunk       fact_chunk;
    WaveDataChunk       data_chunk;
};

#pragma pack(pop)

struct _WaveFile {
    FILE*               fp;
    char*               mode;
    WaveError           error_code;
    WaveMasterChunk     chunk;
};

#ifdef __cplusplus
}
#endif

#endif  /* __WAVE_PRIV_H__ */
