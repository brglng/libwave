#ifndef __WAVE_DEFS_H__
#define __WAVE_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* wave file format codes */
#define WAVE_FORMAT_PCM             0x0001
#define WAVE_FORMAT_IEEE_FLOAT      0x0003
#define WAVE_FORMAT_ALAW            0x0006
#define WAVE_FORMAT_MULAW           0x0007
#define WAVE_FORMAT_EXTENSIBLE      0xfffe

typedef enum _WaveError WaveError;
enum _WaveError {
    WAVE_SUCCESS,           /* no error */
    WAVE_ERROR_FORMAT,      /* not a wave file or unsupported wave format */
    WAVE_ERROR_MODE,        /* incorrect mode when opening the wave file or calling mode-specific API */
    WAVE_ERROR_MALLOC,      /* error when {wave} called malloc */
    WAVE_ERROR_STDIO,       /* error when {wave} called a stdio function */
    WAVE_ERROR_POS,         /* the file pointer is not in the correct position
                               This error should never occur unless there is a bug */
    WAVE_ERROR_PARAM,       /* incorrect parameter passed to the API function */
};

typedef struct _WaveFile WaveFile;

#ifdef __cplusplus
}
#endif

#endif /* __WAVE_DEFS_H__ */
