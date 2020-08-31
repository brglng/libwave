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

#include <stddef.h>
#include <stdint.h>

#if !defined(_MSC_VER) || _MSC_VER >= 1800
#define WAV_CONST const
#else
#define WAV_CONST
#endif

/* wave file format codes */
#define WAV_FORMAT_PCM 0x0001
#define WAV_FORMAT_IEEE_FLOAT 0x0003
#define WAV_FORMAT_ALAW 0x0006
#define WAV_FORMAT_MULAW 0x0007
#define WAV_FORMAT_EXTENSIBLE 0xfffe

typedef enum {
    WAV_OK,         /** no error */
    WAV_ERR_NOMEM,  /** malloc failed */
    WAV_ERR_OS,     /** error when {wave} called a stdio function */
    WAV_ERR_FORMAT, /** not a wave file or unsupported wave format */
    WAV_ERR_MODE,   /** incorrect mode when opening the wave file or calling mode-specific API */
    WAV_ERR_PARAM,  /** incorrect parameter passed to the API function */
} WavErr;

typedef struct {
    void*   (*aligned_alloc)(void *context, size_t alignment, size_t size);
    void*   (*realloc)(void *context, void *p, size_t size);
    void    (*free)(void *context, void *p);
} WavAllocFuncs;

void wav_set_allocator(void *context, WAV_CONST WavAllocFuncs *funcs);

void* wav_aligned_alloc(size_t alignment, size_t size);
void* wav_realloc(void *p, size_t size);
void wav_free(void *p);

#ifdef __cplusplus
#define WAV_ALIGNOF(type) alignof(type)
#else
#if __STDC_VERSION__ >= 201112L
#define WAV_ALIGNOF(type) _Alignof(type)
#else
#define WAV_ALIGNOF(type) offsetof(struct { char c; type d; }, d)
#endif
#endif

#define _WAV_NEW0(_type) wav_aligned_alloc(WAV_ALIGNOF(_type), sizeof(_type))
#define _WAV_NEW1(_type, _count) wav_aligned_alloc(WAV_ALIGNOF(_type), sizeof(_type) * (_count))
#define _WAV_NEW(_type, _0, _macro_name, ...) _macro_name
#define WAV_NEW(_type, ...) ((_type*)_WAV_NEW(_type, ##__VA_ARGS__, _WAV_NEW1, _WAV_NEW0)(_type, ##__VA_ARGS__))

char* wav_strdup(WAV_CONST char *str);

typedef struct _WavFile WavFile;

/** Open a wav file
 *
 *  @param filename     The name of the wav file
 *  @param mode         The mode for open (same as {fopen})
 *  @return             NULL if the memory allocation for the {WavFile} object failed. Non-NULL means the memory allocation succeeded, but there can be other errors, which can be obtained using {wav_errno} or {wav_error}.
 */
WavFile* wav_open(WAV_CONST char* filename, WAV_CONST char* mode);
int      wav_close(WavFile* self);
WavFile* wav_reopen(WavFile* self, WAV_CONST char* filename, WAV_CONST char* mode);

/** Read a block of samples from the wav file
 *
 *  @param buffers      A list of pointers to a buffer where the data of each single channel will be placed
 *  @param count        The number of frames (block size)
 *  @param self         The pointer to the {WavFile} structure
 *  @return             The number of frames read. If returned value is less than {count}, either EOF reached or an error occured
 *  @remarks            This API does not support extensible format. For extensible format, use {wave_read_raw} instead.
 */
size_t wav_read(WavFile* self, void** buffers, size_t count);

/** Write a block of samples to the wav file
 *
 *  @param buffers  A list of pointers to a buffer of the data of a single channel.
 *  @param count    The number of frames (block size)
 *  @param self     The pointer to the {WavFile} structure
 *  @return         The number of frames written. If returned value is less than {count}, either EOF reached or an error occured.
 *  @remarks        This API does not support extensible format. For extensible format, use {wave_read_raw} instead.
 */
size_t wav_write(WavFile* self, WAV_CONST void* WAV_CONST* buffers, size_t count);

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

/** Tell if an error occurred in the last operation
 *
 *  @param self     The pointer to the {WavFile} structure
 *  @return         Non-zero if there is an error, otherwise zero
 */
int wav_error(WAV_CONST WavFile* self);

int wav_flush(WavFile* self);

/** Get the error code of the last operation.
 *
 *  @param self     The pointer to the {WavFile} structure.
 *  @return         A WavErr value, see {WavErr}.
 */
WavErr wav_errno(WAV_CONST WavFile* self);

/** Set the format code
 *
 *  @param self     The {WavFile} object
 *  @param format   The format code, which should be one of `WAV_FORMAT_*`
 *  @remarks        All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.a
 */
void wav_set_format(WavFile* self, uint16_t format);

/** Set the number of channels
 *
 *  @param self             The {WavFile} object
 *  @param num_channels     The number of channels
 *  @remarks                All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_num_channels(WavFile* self, uint16_t num_channels);

/** Set the sample rate
 *
 *  @param self             The {WavFile} object
 *  @param sample_rate      The sample rate
 *  @remarks                All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_sample_rate(WavFile* self, uint32_t sample_rate);

/** Get the number of valid bits per sample
 *
 *  @param self     The {WavFile} object
 *  @param bits     The value of valid bits to set
 *  @remarks        If {bits} is 0 or larger than 8*{sample_size}, an error will occur. All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_valid_bits_per_sample(WavFile* self, uint16_t bits);

/** Set the size (in bytes) per sample
 *
 *  @param self             The WaveFile object
 *  @param sample_size      Number of bytes per sample
 *  @remarks                When this function is called, the {BitsPerSample} and {ValidBitsPerSample} fields in the wav file will be set to 8*{sample_size}. All data will be cleared after the call. {wav_errno} can be used to get the error code if there is an error.
 */
void wav_set_sample_size(WavFile* self, size_t sample_size);

uint16_t wav_get_format(WAV_CONST WavFile* self);
uint16_t wav_get_num_channels(WAV_CONST WavFile* self);
uint32_t wav_get_sample_rate(WAV_CONST WavFile* self);
uint16_t wav_get_valid_bits_per_sample(WAV_CONST WavFile* self);
size_t   wav_get_sample_size(WAV_CONST WavFile* self);
size_t   wav_get_length(WAV_CONST WavFile* self);
uint32_t wav_get_channel_mask(WAV_CONST WavFile* self);
uint16_t wav_get_sub_format(WAV_CONST WavFile* self);

#ifdef __cplusplus
}
#endif

#endif /* __WAV_H__ */
