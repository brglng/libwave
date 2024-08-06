/** Simple PCM wave file I/O library
 *
 * Author: Zhaosheng Pan <zhaosheng.pan@sololand.moe>
 *
 * The API is designed to be similar to stdio.
 *
 * This library does not support:
 *
 *   - formats other than PCM, IEEE float and log-PCM
 *   - extra chunks after the data chunk
 *   - big endian platforms (might be supported in the future)
 */

#ifndef __WAVE_H__
#define __WAVE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

#if !defined(_MSC_VER) || _MSC_VER >= 1800
#define WAVE_INLINE static inline
#define WAVE_CONST const
#define WAVE_RESTRICT restrict
#else
#define WAVE_INLINE static __inline
#define WAVE_CONST
#define WAVE_RESTRICT __restrict
#endif

#if defined(_MSC_VER)
    #define WAVE_API __declspec(dllexport)
#else
    #define WAVE_API __attribute__((visibility("default")))
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__APPLE__)
typedef long long           WaveI64;
typedef unsigned long long  WaveU64;
typedef long long           WaveIntPtr;
typedef unsigned long long  WaveUIntPtr;
#elif defined(__x86_64) || defined(__amd64)  || defined(__aarch64__)
typedef long                WaveI64;
typedef unsigned long       WaveU64;
typedef long                WaveIntPtr;
typedef unsigned long       WaveUIntPtr;
#else
typedef long long           WaveI64;
typedef unsigned long long  WaveU64;
typedef long long           WaveIntPtr;
typedef unsigned long long  WaveUIntPtr;
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
#define WAVE_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define WAVE_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define WAVE_THREAD_LOCAL __declspec(thread)
#else
#define WAVE_THREAD_LOCAL __thread
#endif

typedef int                 WaveBool;
typedef signed char         WaveI8;
typedef unsigned char       WaveU8;
typedef short               WaveI16;
typedef unsigned short      WaveU16;
typedef int                 WaveI32;
typedef unsigned int        WaveU32;

enum {
    WAVE_FALSE,
    WAVE_TRUE
};

/* wave file format codes */

// Windows already has these macros in mmreg.h
#if !((defined(_WIN32) || defined(_WIN64)) && defined(WAVE_FORMAT_PCM))
#define WAVE_FORMAT_PCM          ((WaveU16)0x0001)
#endif

#if !((defined(_WIN32) || defined(_WIN64)) && defined(WAVE_FORMAT_IEEE_FLOAT))
#define WAVE_FORMAT_IEEE_FLOAT   ((WaveU16)0x0003)
#endif

#if !((defined(_WIN32) || defined(_WIN64)) && defined(WAVE_FORMAT_ALAW))
#define WAVE_FORMAT_ALAW         ((WaveU16)0x0006)
#endif

#if !((defined(_WIN32) || defined(_WIN64)) && defined(WAVE_FORMAT_MULAW))
#define WAVE_FORMAT_MULAW        ((WaveU16)0x0007)
#endif

#if !((defined(_WIN32) || defined(_WIN64)) && defined(WAVE_FORMAT_EXTENSIBLE))
#define WAVE_FORMAT_EXTENSIBLE   ((WaveU16)0xfffe)
#endif

typedef enum {
    WAVE_OK,         /** no error */
    WAVE_ERR_OS,     /** error when {wave} called a stdio function */
    WAVE_ERR_FORMAT, /** not a wave file or unsupported wave format */
    WAVE_ERR_MODE,   /** incorrect mode when opening the wave file or calling mode-specific API */
    WAVE_ERR_PARAM,  /** incorrect parameter passed to the API function */
} WaveErrCode;

typedef struct {
    WaveErrCode     code;
    char*           message;
    int             _is_literal;
} WaveErr;

typedef struct {
    void*   (*malloc)(void *context, size_t size);
    void*   (*realloc)(void *context, void *p, size_t size);
    void    (*free)(void *context, void *p);
} WaveAllocFuncs;

void wave_set_allocator(void *context, WAVE_CONST WaveAllocFuncs *funcs);

void* wave_malloc(size_t size);
void* wave_realloc(void *p, size_t size);
void wave_free(void *p);

char* wave_strdup(WAVE_CONST char *str);
char* wave_strndup(WAVE_CONST char *str, size_t n);
int wave_vasprintf(char **str, WAVE_CONST char *format, va_list args);
int wave_asprintf(char **str, WAVE_CONST char *format, ...);

WAVE_API WAVE_CONST WaveErr* wave_err(void);
WAVE_API void wave_err_clear(void);

#define WAVE_OPEN_READ       1
#define WAVE_OPEN_WRITE      2
#define WAVE_OPEN_APPEND     4

typedef struct _WaveFile WaveFile;

/** Open a wav file
 *
 *  @param filename     The name of the wav file
 *  @param mode         The mode for open (same as {fopen})
 *  @return             NULL if the memory allocation for the {WaveFile} object failed. Non-NULL means the memory allocation succeeded, but there can be other errors, which can be obtained using {wave_err}.
 */
WAVE_API WaveFile* wave_open(WAVE_CONST char* filename, WaveU32 mode);
WAVE_API void     wave_close(WaveFile* self);
WAVE_API WaveFile* wave_reopen(WaveFile* self, WAVE_CONST char* filename, WaveU32 mode);

/** Read a block of samples from the wav file
 *
 *  @param buffer       A pointer to a buffer where the data will be placed
 *  @param count        The number of frames (block size)
 *  @param self         The pointer to the {WaveFile} structure
 *  @return             The number of frames read. If returned value is less than {count}, either EOF reached or an error occured
 *  @remarks            This API does not support extensible format. For extensible format, use {wave_read_raw} instead.
 */
WAVE_API size_t wave_read(WaveFile* self, void *buffer, size_t count);

/** Write a block of samples to the wav file
 *
 *  @param buffer   A pointer to the buffer of data
 *  @param count    The number of frames (block size)
 *  @param self     The pointer to the {WaveFile} structure
 *  @return         The number of frames written. If returned value is less than {count}, either EOF reached or an error occured.
 *  @remarks        This API does not support extensible format. For extensible format, use {wave_read_raw} instead.
 */
WAVE_API size_t wave_write(WaveFile* self, WAVE_CONST void *buffer, size_t count);

/** Tell the current position in the wav file.
 *
 *  @param self     The pointer to the WaveFile structure.
 *  @return         The current frame index.
 */
WAVE_API long int wave_tell(WAVE_CONST WaveFile* self);

WAVE_API int  wave_seek(WaveFile* self, long int offset, int origin);
WAVE_API void wave_rewind(WaveFile* self);

/** Tell if the end of the wav file is reached.
 *
 *  @param self     The pointer to the WaveFile structure.
 *  @return         Non-zero integer if the end of the wav file is reached, otherwise zero.
 */
WAVE_API int wave_eof(WAVE_CONST WaveFile* self);

WAVE_API int wave_flush(WaveFile* self);

/** Set the format code
 *
 *  @param self     The {WaveFile} object
 *  @param format   The format code, which should be one of `WAVE_FORMAT_*`
 *  @remarks        All data will be cleared after the call. {wave_err} can be used to get the error code if there is an error.
 */
WAVE_API void wave_set_format(WaveFile* self, WaveU16 format);

/** Set the number of channels
 *
 *  @param self             The {WaveFile} object
 *  @param num_channels     The number of channels
 *  @remarks                All data will be cleared after the call. {wave_err} can be used to get the error code if there is an error.
 */
WAVE_API void wave_set_num_channels(WaveFile* self, WaveU16 num_channels);

/** Set the sample rate
 *
 *  @param self             The {WaveFile} object
 *  @param sample_rate      The sample rate
 *  @remarks                All data will be cleared after the call. {wave_err} can be used to get the error code if there is an error.
 */
WAVE_API void wave_set_sample_rate(WaveFile* self, WaveU32 sample_rate);

/** Get the number of valid bits per sample
 *
 *  @param self     The {WaveFile} object
 *  @param bits     The value of valid bits to set
 *  @remarks        If {bits} is 0 or larger than 8*{sample_size}, an error will occur. All data will be cleared after the call. {wave_err} can be used to get the error code if there is an error.
 */
WAVE_API void wave_set_valid_bits_per_sample(WaveFile* self, WaveU16 bits);

/** Set the size (in bytes) per sample
 *
 *  @param self             The WaveeFile object
 *  @param sample_size      Number of bytes per sample
 *  @remarks                When this function is called, the {BitsPerSample} and {ValidBitsPerSample} fields in the wav file will be set to 8*{sample_size}. All data will be cleared after the call. {wave_err} can be used to get the error code if there is an error.
 */
WAVE_API void wave_set_sample_size(WaveFile* self, size_t sample_size);

WAVE_API WaveU16 wave_get_format(WAVE_CONST WaveFile* self);
WAVE_API WaveU16 wave_get_num_channels(WAVE_CONST WaveFile* self);
WAVE_API WaveU32 wave_get_sample_rate(WAVE_CONST WaveFile* self);
WAVE_API WaveU16 wave_get_valid_bits_per_sample(WAVE_CONST WaveFile* self);
WAVE_API size_t wave_get_sample_size(WAVE_CONST WaveFile* self);
WAVE_API size_t wave_get_length(WAVE_CONST WaveFile* self);
WAVE_API WaveU32 wave_get_channel_mask(WAVE_CONST WaveFile* self);
WAVE_API WaveU16 wave_get_sub_format(WAVE_CONST WaveFile* self);

#ifdef __cplusplus
}
#endif

#endif /* __WAVE_H__ */
