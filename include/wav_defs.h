#ifndef __WAV_DEFS_H__
#define __WAV_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* wave file format codes */
#define WAV_FORMAT_PCM 0x0001
#define WAV_FORMAT_IEEE_FLOAT 0x0003
#define WAV_FORMAT_ALAW 0x0006
#define WAV_FORMAT_MULAW 0x0007
#define WAV_FORMAT_EXTENSIBLE 0xfffe

typedef enum _WavError WavError;
enum _WavError {
    WAV_OK,           /* no error */
    WAV_ERROR_FORMAT, /* not a wave file or unsupported wave format */
    WAV_ERROR_MODE,   /* incorrect mode when opening the wave file or calling mode-specific API */
    WAV_ERROR_MALLOC, /* error when {wave} called malloc */
    WAV_ERROR_STDIO,  /* error when {wave} called a stdio function */
    WAV_ERROR_PARAM,  /* incorrect parameter passed to the API function */
};

typedef struct _WavFile WavFile;

#ifdef __cplusplus
}
#endif

#endif /* __WAV_DEFS_H__ */
