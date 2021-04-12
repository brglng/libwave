/** Simple PCM wav file I/O library
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

#ifndef __WAV_H__
#define __WAV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

#if !defined(_MSC_VER) || _MSC_VER >= 1800
#define WAV_INLINE static inline
#define WAV_CONST const
#define WAV_RESTRICT restrict
#else
#define WAV_INLINE static __inline
#define WAV_CONST
#define WAV_RESTRICT __restrict
#endif

#if defined(__APPLE__) || defined(_MSC_VER)
typedef long long           WavI64;
typedef unsigned long long  WavU64;
typedef long long           WavIntPtr;
typedef unsigned long long  WavUIntPtr;
#else
#if defined(_WIN64) || defined(__x86_64) || defined(__amd64)
typedef long                WavI64;
typedef unsigned long       WavU64;
typedef long                WavIntPtr;
typedef unsigned long       WavUIntPtr;
#else
typedef long long           WavI64;
typedef unsigned long long  WavU64;
typedef int                 WavIntPtr;
typedef unsigned int        WavUIntPtr;
#endif
#endif

#if defined(__cplusplus) && __cplusplus >= 201103L
#define WAV_THREAD_LOCAL thread_local
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define WAV_THREAD_LOCAL _Thread_local
#elif defined(_MSC_VER)
#define WAV_THREAD_LOCAL __declspec(thread)
#else
#define WAV_THREAD_LOCAL __thread
#endif

typedef int                 WavBool;
typedef signed char         WavI8;
typedef unsigned char       WavU8;
typedef short               WavI16;
typedef unsigned short      WavU16;
typedef int                 WavI32;
typedef unsigned int        WavU32;

enum {
    WAV_FALSE,
    WAV_TRUE
};

/* wave file format codes */
#define WAV_FORMAT_PCM          ((WavU16)0x0001)
#define WAV_FORMAT_IEEE_FLOAT   ((WavU16)0x0003)
#define WAV_FORMAT_ALAW         ((WavU16)0x0006)
#define WAV_FORMAT_MULAW        ((WavU16)0x0007)
#define WAV_FORMAT_EXTENSIBLE   ((WavU16)0xfffe)

typedef enum {
    WAV_OK,         /** no error */
    WAV_ERR_OS,     /** error when {wave} called a stdio function */
    WAV_ERR_FORMAT, /** not a wave file or unsupported wave format */
    WAV_ERR_MODE,   /** incorrect mode when opening the wave file or calling mode-specific API */
    WAV_ERR_PARAM,  /** incorrect parameter passed to the API function */
} WavErrCode;

typedef struct {
    WavErrCode  code;
    char*       message;
    int         _is_literal;
} WavErr;

typedef struct {
    void*   (*malloc)(void *context, size_t size);
    void*   (*realloc)(void *context, void *p, size_t size);
    void    (*free)(void *context, void *p);
} WavAllocFuncs;

void wav_set_allocator(void *context, WAV_CONST WavAllocFuncs *funcs);

void* wav_malloc(size_t size);
void* wav_realloc(void *p, size_t size);
void wav_free(void *p);

char* wav_strdup(WAV_CONST char *str);
char* wav_strndup(WAV_CONST char *str, size_t n);
int wav_vasprintf(char **str, WAV_CONST char *format, va_list args);
int wav_asprintf(char **str, WAV_CONST char *format, ...);

WAV_CONST WavErr* wav_err(void);
void wav_err_clear(void);

#define WAV_OPEN_READ       1
#define WAV_OPEN_WRITE      2
#define WAV_OPEN_APPEND     4

typedef struct _WavFile WavFile;

/** Open a wav file
 *
 *  @param filename     The name of the wav file
 *  @param mode         The mode for open (same as {fopen})
 *  @return             NULL if the memory allocation for the {WavFile} object failed. Non-NULL means the memory allocation succeeded, but there can be other errors, which can be obtained using {wav_errno} or {wav_error}.
 */
WavFile* wav_open(WAV_CONST char* filename, WavU32 mode);
void     wav_close(WavFile* self);
WavFile* wav_reopen(WavFile* self, WAV_CONST char* filename, WavU32 mode);

/** Read a block of samples from the wav file
 *
 *  @param buffer       A pointer to a buffer where the data will be placed
 *  @param count        The number of frames (block size)
 *  @param self         The pointer to the {WavFile} structure
 *  @return             The number of frames read. If returned value is less than {count}, either EOF reached or an error occured
 *  @remarks            This API does not support extensible format. For extensible format, use {wave_read_raw} instead.
 */
size_t wav_read(WavFile* self, void *buffer, size_t count);

/** Write a block of samples to the wav file
 *
 *  @param buffer   A pointer to the buffer of data
 *  @param count    The number of frames (block size)
 *  @param self     The pointer to the {WavFile} structure
 *  @return         The number of frames written. If returned value is less than {count}, either EOF reached or an error occured.
 *  @remarks        This API does not support extensible format. For extensible format, use {wave_read_raw} instead.
 */
size_t wav_write(WavFile* self, WAV_CONST void *buffer, size_t count);

/** Tell the current position in the wav file.
 *
 *  @param self     The pointer to the WavFile structure.
 *  @return         The current frame index.
 */
long int wav_tell(WAV_CONST WavFile* self);

int  wav_seek(WavFile* self, long int offset, int origin);
void wav_rewind(WavFile* self);

/** Tell if the end of the wav file is reached.
 *
 *  @param self     The pointer to the WavFile structure.
 *  @return         Non-zero integer if the end of the wav file is reached, otherwise zero.
 */
int wav_eof(WAV_CONST WavFile* self);

int wav_flush(WavFile* self);

/** Set the format code
 *
 *  @param self     The {WavFile} object
 *  @param format   The format code, which should be one of `WAV_FORMAT_*`
 *  @remarks        All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_format(WavFile* self, WavU16 format);

/** Set the number of channels
 *
 *  @param self             The {WavFile} object
 *  @param num_channels     The number of channels
 *  @remarks                All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_num_channels(WavFile* self, WavU16 num_channels);

/** Set the sample rate
 *
 *  @param self             The {WavFile} object
 *  @param sample_rate      The sample rate
 *  @remarks                All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_sample_rate(WavFile* self, WavU32 sample_rate);

/** Get the number of valid bits per sample
 *
 *  @param self     The {WavFile} object
 *  @param bits     The value of valid bits to set
 *  @remarks        If {bits} is 0 or larger than 8*{sample_size}, an error will occur. All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_valid_bits_per_sample(WavFile* self, WavU16 bits);

/** Set the size (in bytes) per sample
 *
 *  @param self             The WaveFile object
 *  @param sample_size      Number of bytes per sample
 *  @remarks                When this function is called, the {BitsPerSample} and {ValidBitsPerSample} fields in the wav file will be set to 8*{sample_size}. All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_sample_size(WavFile* self, size_t sample_size);

WavU16 wav_get_format(WAV_CONST WavFile* self);
WavU16 wav_get_num_channels(WAV_CONST WavFile* self);
WavU32 wav_get_sample_rate(WAV_CONST WavFile* self);
WavU16 wav_get_valid_bits_per_sample(WAV_CONST WavFile* self);
size_t wav_get_sample_size(WAV_CONST WavFile* self);
size_t wav_get_length(WAV_CONST WavFile* self);
WavU32 wav_get_channel_mask(WAV_CONST WavFile* self);
WavU16 wav_get_sub_format(WAV_CONST WavFile* self);

#ifdef __cplusplus
}
#endif

#endif /* __WAV_H__ */
