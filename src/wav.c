#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wav.h"

#if defined(__x86_64) || defined(__amd64) || defined(__i386__) || defined(__x86_64__) || defined(__LITTLE_ENDIAN__)
#define WAV_ENDIAN_LITTLE 1
#elif defined(__BIG_ENDIAN__)
#define WAV_ENDIAN_BIG 1
#endif

#if WAV_ENDIAN_LITTLE
#define WAV_RIFF_CHUNK_ID       ((WavU32)'FFIR')
#define WAV_FORMAT_CHUNK_ID     ((WavU32)' tmf')
#define WAV_FACT_CHUNK_ID       ((WavU32)'tcaf')
#define WAV_DATA_CHUNK_ID       ((WavU32)'atad')
#define WAV_WAVE_ID             ((WavU32)'EVAW')
#endif

#if WAV_ENDIAN_BIG
#define WAV_RIFF_CHUNK_ID       ((WavU32)'RIFF')
#define WAV_FORMAT_CHUNK_ID     ((WavU32)'fmt ')
#define WAV_FACT_CHUNK_ID       ((WavU32)'fact')
#define WAV_DATA_CHUNK_ID       ((WavU32)'data')
#define WAV_WAVE_ID             ((WavU32)'WAVE')
#endif

static WAV_THREAD_LOCAL WavErr g_err = {WAV_OK, "", 1};

static void* wav_default_aligned_alloc(void *context, size_t alignment, size_t size)
{
    (void)context;
    alignment = (alignment + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
#ifdef _WIN32
    void *p = _aligned_malloc(size, alignment);
    assert(p != NULL);
#else
    void *p = NULL;
    int err = posix_memalign(&p, alignment, size);
    assert(err == 0);
#endif
    return p;
}

static void* wav_default_realloc(void *context, void *p, size_t size)
{
    (void)context;
    void *ptr = realloc(p, size);
    assert(ptr != NULL);
    return ptr;
}

static void wav_default_free(void *context, void *p)
{
    (void)context;
    free(p);
}

static WavAllocFuncs g_default_alloc_funcs = {
    &wav_default_aligned_alloc,
    &wav_default_realloc,
    &wav_default_free
};

static void *g_alloc_context = NULL;
static WAV_CONST WavAllocFuncs* g_alloc_funcs = &g_default_alloc_funcs;

void wav_set_allocator(void *context, WAV_CONST WavAllocFuncs *funcs)
{
    g_alloc_context = context;
    g_alloc_funcs = funcs;
}

void* wav_aligned_alloc(size_t alignment, size_t size)
{
    return g_alloc_funcs->aligned_alloc(g_alloc_context, alignment, size);
}

void* wav_realloc(void *p, size_t size)
{
    return g_alloc_funcs->realloc(g_alloc_context, p, size);
}

void wav_free(void *p)
{
    if (p != NULL) {
        g_alloc_funcs->free(g_alloc_context, p);
    }
}

char* wav_strdup(WAV_CONST char *str)
{
    size_t len = strlen(str) + 1;
    void *new = WAV_NEW(char, len);
    if (new == NULL)
        return NULL;

    return memcpy(new, str, len);
}

char* wav_strndup(WAV_CONST char *str, size_t n)
{
    char *result = WAV_NEW(char, n + 1);
    if (result == NULL)
        return NULL;

    result[n] = 0;
    return memcpy(result, str, n);
}

int wav_vasprintf(char **str, WAV_CONST char *format, va_list args)
{
    int size = 0;

    va_list args_copy;
    va_copy(args_copy, args);
    size = vsnprintf(NULL, (size_t)size, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        return size;
    }

    *str = WAV_NEW(char, (size_t)size + 1);
    if (*str == NULL)
        return -1;

    return vsprintf(*str, format, args);
}

int wav_asprintf(char **str, WAV_CONST char *format, ...)
{
    va_list args;
    va_start(args, format);
    int size = wav_vasprintf(str, format, args);
    va_end(args);
    return size;
}

WAV_CONST WavErr* wav_err(void)
{
    return &g_err;
}

void wav_err_clear(void)
{
    if (g_err.code != WAV_OK) {
        if (!g_err._is_literal) {
            wav_free(g_err.message);
        }
        g_err.code = WAV_OK;
        g_err.message = "";
        g_err._is_literal = 1;
    }
}

WAV_INLINE void wav_err_set(WavErrCode code, WAV_CONST char *format, ...)
{
    assert(g_err.code == WAV_OK);
    va_list args;
    va_start(args, format);
    g_err.code = code;
    wav_vasprintf(&g_err.message, format, args);
    g_err._is_literal = 0;
    va_end(args);
}

WAV_INLINE void wav_err_set_literal(WavErrCode code, WAV_CONST char *message)
{
    assert(g_err.code == WAV_OK);
    g_err.code = code;
    g_err.message = (char *)message;
    g_err._is_literal = 1;
}

#pragma pack(push, 1)

#define WAV_RIFF_HEADER_SIZE 8

typedef struct {
    /* RIFF header */
    WavU32 id;
    WavU32 size;

    WavU16 format_tag;
    WavU16 n_channels;
    WavU32 sample_rate;
    WavU32 avg_bytes_per_sec;
    WavU16 block_align;
    WavU16 bits_per_sample;

    WavU16 ext_size;
    WavU16 valid_bits_per_sample;
    WavU32 channel_mask;

    WavU8 sub_format[16];
} WavFormatChunk;

typedef struct {
    /* RIFF header */
    WavU32 id;
    WavU32 size;

    WavU32 sample_length;
} WavFactChunk;

typedef struct {
    /* RIFF header */
    WavU32 id;
    WavU32 size;
} WavDataChunk;

typedef struct {
    /* RIFF header */
    WavU32 id;
    WavU32 size;

    WavU32 wave_id;

    WavFormatChunk format_chunk;
    WavFactChunk   fact_chunk;
    WavDataChunk   data_chunk;
} WavMasterChunk;

#pragma pack(pop)

struct _WavFile {
    FILE*               fp;
    char*               filename;
    WAV_CONST char*     mode;
    WavMasterChunk      chunk;
};

static WAV_CONST WavU8 default_sub_format[16] = {
    0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

size_t wav_get_header_size(WAV_CONST WavFile* self)
{
    size_t header_size = WAV_RIFF_HEADER_SIZE + 4 +
                         WAV_RIFF_HEADER_SIZE + self->chunk.format_chunk.size +
                         WAV_RIFF_HEADER_SIZE;

    if (self->chunk.fact_chunk.id == WAV_FACT_CHUNK_ID) {
        header_size += WAV_RIFF_HEADER_SIZE + self->chunk.fact_chunk.size;
    }

    return header_size;
}

void wav_parse_header(WavFile* self)
{
    size_t read_count;

    read_count = fread(&self->chunk, WAV_RIFF_HEADER_SIZE, 1, self->fp);
    if (read_count != 1) {
        wav_err_set_literal(WAV_ERR_FORMAT, "unexpected EOF");
        return;
    }

    if (self->chunk.id != WAV_RIFF_CHUNK_ID) {
        wav_err_set(WAV_ERR_FORMAT, "invalid RIFF chunk ID: %#010x", self->chunk.id);
        return;
    }

    read_count = fread(&self->chunk.wave_id, 4, 1, self->fp);
    if (read_count != 1) {
        wav_err_set_literal(WAV_ERR_FORMAT, "unexpected EOF");
        return;
    }
    if (self->chunk.wave_id != WAV_WAVE_ID) {
        wav_err_set(WAV_ERR_FORMAT, "invalid WAVE ID: %#010x", self->chunk.wave_id);
        return;
    }

    read_count = fread(&self->chunk.format_chunk, WAV_RIFF_HEADER_SIZE, 1, self->fp);
    if (read_count != 1) {
        wav_err_set_literal(WAV_ERR_FORMAT, "unexpected EOF");
        return;
    }
    if (self->chunk.format_chunk.id != WAV_FORMAT_CHUNK_ID) {
        wav_err_set(WAV_ERR_FORMAT, "invalid FORMAT chunk ID: %#010x", self->chunk.format_chunk.id);
        return;
    }

    read_count = fread((WavU8*)(&self->chunk.format_chunk) + WAV_RIFF_HEADER_SIZE,
                       self->chunk.format_chunk.size, 1, self->fp);
    if (read_count != 1) {
        wav_err_set_literal(WAV_ERR_FORMAT, "unexpected EOF");
        return;
    }

    if (self->chunk.format_chunk.format_tag == WAV_FORMAT_PCM) {
    } else if (self->chunk.format_chunk.format_tag == WAV_FORMAT_IEEE_FLOAT) {
    } else if (self->chunk.format_chunk.format_tag == WAV_FORMAT_ALAW ||
               self->chunk.format_chunk.format_tag == WAV_FORMAT_MULAW) {
    } else {
        wav_err_set(WAV_ERR_FORMAT, "invalid format tag: %010x", self->chunk.format_chunk.format_tag);
        return;
    }

    read_count = fread(&self->chunk.fact_chunk, WAV_RIFF_HEADER_SIZE, 1, self->fp);
    if (read_count != 1) {
        wav_err_set(WAV_ERR_FORMAT, "unexpected EOF");
        return;
    }

    if (self->chunk.fact_chunk.id == WAV_FACT_CHUNK_ID) {
        read_count = fread((WavU8*)(&self->chunk.fact_chunk) + WAV_RIFF_HEADER_SIZE,
                           self->chunk.fact_chunk.size, 1, self->fp);
        if (read_count != 1) {
            wav_err_set(WAV_ERR_FORMAT, "unexpected EOF");
            return;
        }

        read_count = fread(&self->chunk.data_chunk, WAV_RIFF_HEADER_SIZE, 1, self->fp);
        if (read_count != 1) {
            wav_err_set(WAV_ERR_FORMAT, "unexpected EOF");
            return;
        }
    } else if (self->chunk.fact_chunk.id == WAV_DATA_CHUNK_ID) {
        self->chunk.data_chunk.id   = self->chunk.fact_chunk.id;
        self->chunk.data_chunk.size = self->chunk.fact_chunk.size;
        self->chunk.fact_chunk.size = 0;
    }
}

void wav_write_header(WavFile* self)
{
    size_t write_count;

    fseek(self->fp, 0, SEEK_SET);

    self->chunk.size = (4) +
                       (WAV_RIFF_HEADER_SIZE + self->chunk.format_chunk.size) +
                       (WAV_RIFF_HEADER_SIZE + self->chunk.data_chunk.size);

    if (self->chunk.fact_chunk.id == WAV_FACT_CHUNK_ID) {
        /* if there is a fact chunk */
        self->chunk.size += WAV_RIFF_HEADER_SIZE + self->chunk.fact_chunk.size;
    }

    if (self->chunk.size % 2 != 0) {
        self->chunk.size++;
    }

    write_count = fwrite(&self->chunk, WAV_RIFF_HEADER_SIZE + 4, 1, self->fp);
    if (write_count != 1) {
        wav_err_set(WAV_ERR_OS, "Error while writing to %s: %s [OS error %d]", self->filename, strerror(errno), errno);
        return;
    }

    write_count = fwrite(&self->chunk.format_chunk, WAV_RIFF_HEADER_SIZE + self->chunk.format_chunk.size, 1, self->fp);
    if (write_count != 1) {
        wav_err_set(WAV_ERR_OS, "Error while writing to %s: %s [OS error %d]", self->filename, strerror(errno), errno);
        return;
    }

    if (self->chunk.fact_chunk.id == WAV_FACT_CHUNK_ID) {
        /* if there is a fact chunk */
        write_count = fwrite(&self->chunk.fact_chunk, WAV_RIFF_HEADER_SIZE + self->chunk.fact_chunk.size, 1, self->fp);
        if (write_count != 1) {
            wav_err_set(WAV_ERR_OS, "Error while writing to %s: %s [OS error %d]", self->filename, strerror(errno), errno);
            return;
        }
    }

    write_count = fwrite(&self->chunk.data_chunk, WAV_RIFF_HEADER_SIZE, 1, self->fp);
    if (write_count != 1) {
        wav_err_set(WAV_ERR_OS, "Error while writing to %s: %s [OS error %d]", self->filename, strerror(errno), errno);
        return;
    }
}

void wav_init(WavFile* self, WAV_CONST char* filename, WAV_CONST char* mode)
{
    memset(self, 0, sizeof(WavFile));

    if (strncmp(mode, "r", 1) == 0 || strncmp(mode, "rb", 2) == 0) {
        self->mode = "rb";
    } else if (strncmp(mode, "r+", 2) == 0 || strncmp(mode, "rb+", 3) == 0 || strncmp(mode, "r+b", 3) == 0) {
        self->mode = "rb+";
    } else if (strncmp(mode, "w", 1) == 0 || strncmp(mode, "wb", 2) == 0) {
        self->mode = "wb";
    } else if (strncmp(mode, "w+", 2) == 0 || strncmp(mode, "wb+", 3) == 0 || strncmp(mode, "w+b", 3) == 0) {
        self->mode = "wb+";
    } else if (strncmp(mode, "wx", 2) == 0 || strncmp(mode, "wbx", 3) == 0) {
        self->mode = "wbx";
    } else if (strncmp(mode, "w+x", 3) == 0 || strncmp(mode, "wb+x", 4) == 0 || strncmp(mode, "w+bx", 4) == 0) {
        self->mode = "wb+x";
    } else if (strncmp(mode, "a", 1) == 0 || strncmp(mode, "ab", 2) == 0) {
        self->mode = "ab";
    } else if (strncmp(mode, "a+", 2) == 0 || strncmp(mode, "ab+", 3) == 0 || strncmp(mode, "a+b", 3) == 0) {
        self->mode = "ab+";
    } else {
        wav_err_set(WAV_ERR_MODE, "Mode is incorrect: %s", mode);
        return;
    }

    self->filename = wav_strdup(filename);

    self->fp = fopen(filename, self->mode);
    if (self->fp == NULL) {
        wav_err_set(WAV_ERR_OS, "Error when opening %s: %s [errno %d]", filename, strerror(errno), errno);
        return;
    }

    if (self->mode[0] == 'r') {
        wav_parse_header(self);
        return;
    }

    if (self->mode[0] == 'a') {
        wav_parse_header(self);
        if (g_err.code == WAV_OK) {
            // If it succeeded in parsing header, return immediately.
            return;
        } else {
            // Header parsing failed. Regard it as a new file.
            rewind(self->fp);
        }
    }

    // reaches here only if (mode[0] == 'w' || mode[0] == 'a')

    self->chunk.id = WAV_RIFF_CHUNK_ID;
    /* self->chunk.size = calculated by wav_write_header */
    self->chunk.wave_id = WAV_WAVE_ID;

    self->chunk.format_chunk.id                = WAV_FORMAT_CHUNK_ID;
    self->chunk.format_chunk.size              = (WavU32)(&self->chunk.format_chunk.ext_size - &self->chunk.format_chunk.format_tag);
    self->chunk.format_chunk.format_tag        = WAV_FORMAT_PCM;
    self->chunk.format_chunk.n_channels        = 2;
    self->chunk.format_chunk.sample_rate       = 44100;
    self->chunk.format_chunk.avg_bytes_per_sec = 44100 * 2 * 2;
    self->chunk.format_chunk.block_align       = 4;
    self->chunk.format_chunk.bits_per_sample   = 16;

    memcpy(self->chunk.format_chunk.sub_format, default_sub_format, 16);

    self->chunk.fact_chunk.id   = WAV_DATA_CHUNK_ID; /* flag to indicate there is no fact chunk */
    self->chunk.fact_chunk.size = 0;

    self->chunk.data_chunk.id   = WAV_DATA_CHUNK_ID;
    self->chunk.data_chunk.size = 0;

    wav_write_header(self);
}

void wav_finalize(WavFile* self)
{
    int ret;

    wav_free(self->filename);

    if (self->fp == NULL) {
        return;
    }

    if ((strncmp(self->mode, "wb", 2) == 0 ||
         strncmp(self->mode, "wb+", 3) == 0 ||
         strncmp(self->mode, "wbx", 3) == 0 ||
         strncmp(self->mode, "wb+x", 4) == 0 ||
         strncmp(self->mode, "ab", 2) == 0 ||
         strncmp(self->mode, "ab+", 3) == 0) &&
        self->chunk.data_chunk.size % 2 != 0 &&
        wav_eof(self)) {
        char padding = 0;
        ret          = (int)fwrite(&padding, sizeof(padding), 1, self->fp);
        if (ret != 1) {
            fprintf(stderr, "libwav: fwrite to %s failed: %s [errno %d]", self->filename, strerror(errno), errno);
            return;
        }
    }

    ret = fclose(self->fp);
    if (ret != 0) {
        fprintf(stderr, "libwav: fclose failed with code %d: %s [errno %d]", ret, strerror(errno), errno);
        return;
    }
}

WavFile* wav_open(WAV_CONST char* filename, WAV_CONST char* mode)
{
    WavFile* self = WAV_NEW(WavFile);
    if (self == NULL) {
        return NULL;
    }

    wav_init(self, filename, mode);

    return self;
}

void wav_close(WavFile* self)
{
    wav_finalize(self);
    wav_free(self);
}

WavFile* wav_reopen(WavFile* self, WAV_CONST char* filename, WAV_CONST char* mode)
{
    wav_finalize(self);
    wav_init(self, filename, mode);
    return self;
}

size_t wav_read(WavFile* self, void *buffer, size_t count)
{
    size_t   read_count;
    WavU16 n_channels     = wav_get_num_channels(self);
    size_t   sample_size    = wav_get_sample_size(self);
    size_t   len_remain;

    if (strncmp(self->mode, "wb", 2) == 0 || strncmp(self->mode, "wbx", 3) == 0 || strncmp(self->mode, "ab", 2) == 0) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not readable");
        return 0;
    }

    if (self->chunk.format_chunk.format_tag == WAV_FORMAT_EXTENSIBLE) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Extensible format is not supported");
        return 0;
    }

    len_remain = wav_get_length(self) - (size_t)wav_tell(self);
    if (g_err.code != WAV_OK) {
        return 0;
    }
    count = (count <= len_remain) ? count : len_remain;

    if (count == 0) {
        return 0;
    }

    read_count = fread(buffer, sample_size, n_channels * count, self->fp);
    if (ferror(self->fp)) {
        wav_err_set(WAV_ERR_OS, "Error when reading %s: %s [errno %d]", self->filename, strerror(errno), errno);
        return 0;
    }

    return read_count / n_channels;
}

size_t wav_write(WavFile* self, WAV_CONST void *buffer, size_t count)
{
    size_t   write_count;
    WavU16 n_channels     = wav_get_num_channels(self);
    size_t   sample_size    = wav_get_sample_size(self);
    long int save_pos;

    if (strncmp(self->mode, "rb", 2) == 0) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return 0;
    }

    if (self->chunk.format_chunk.format_tag == WAV_FORMAT_EXTENSIBLE) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Extensible format is not supported");
        return 0;
    }

    if (count == 0) {
        return 0;
    }

    wav_tell(self);
    if (g_err.code != WAV_OK) {
        return 0;
    }

    write_count = fwrite(buffer, sample_size, n_channels * count, self->fp);
    if (ferror(self->fp)) {
        wav_err_set(WAV_ERR_OS, "Error when writing to %s: %s [errno %d]", self->filename, strerror(errno), errno);
        return 0;
    }

    self->chunk.data_chunk.size += write_count * sample_size;

    if (self->chunk.fact_chunk.id == WAV_FACT_CHUNK_ID) {
        self->chunk.fact_chunk.sample_length += write_count / n_channels;
    }

    save_pos = ftell(self->fp);
    if (save_pos == -1L) {
        wav_err_set(WAV_ERR_OS, "ftell() failed: %s [errno %d]", strerror(errno), errno);
        return 0;
    }
    wav_write_header(self);
    if (g_err.code != WAV_OK) {
        return 0;
    }
    if (fseek(self->fp, save_pos, SEEK_SET) != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed: %s [errno %d]", strerror(errno), errno);
        return 0;
    }

    return write_count / n_channels;
}

long int wav_tell(WAV_CONST WavFile* self)
{
    long pos = ftell(self->fp);

    if (pos == -1L) {
        wav_err_set(WAV_ERR_OS, "ftell() failed: %s [errno %d]", strerror(errno), errno);
        return -1L;
    } else {
        long header_size = (long)wav_get_header_size(self);

        assert(pos >= header_size);

        return (pos - header_size) / (self->chunk.format_chunk.block_align);
    }
}

int wav_seek(WavFile* self, long int offset, int origin)
{
    size_t length = wav_get_length(self);
    int    ret;

    if (origin == SEEK_CUR) {
        offset += wav_tell(self);
    } else if (origin == SEEK_END) {
        offset += length;
    }

    if (offset >= 0 && (size_t)offset <= length) {
        offset = self->chunk.format_chunk.block_align;
    } else {
        wav_err_set_literal(WAV_ERR_PARAM, "Invalid seek");
        return (int)g_err.code;
    }

    ret = fseek(self->fp, offset, SEEK_SET);

    if (ret != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed: %s [errno %d]", strerror(errno), errno);
        return (int)g_err.code;
    }

    return 0;
}

void wav_rewind(WavFile* self)
{
    wav_seek(self, 0, SEEK_SET);
}

int wav_eof(WAV_CONST WavFile* self)
{
    return feof(self->fp) ||
           ftell(self->fp) == (long)(wav_get_header_size(self) + self->chunk.data_chunk.size);
}

int wav_flush(WavFile* self)
{
    int ret = fflush(self->fp);

    if (ret != 0) {
        wav_err_set(WAV_ERR_OS, "fflush() failed: %s [errno %d]", strerror(errno), errno);
    }

    return ret;
}

void wav_set_format(WavFile* self, WavU16 format)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    self->chunk.format_chunk.format_tag = format;
    if (format != WAV_FORMAT_PCM && format != WAV_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.ext_size = 0;
        self->chunk.format_chunk.size     = (WavU32)(&self->chunk.format_chunk.valid_bits_per_sample - &self->chunk.format_chunk.format_tag);
    } else if (format == WAV_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.ext_size = 22;
        self->chunk.format_chunk.size     = sizeof(WavFormatChunk) - WAV_RIFF_HEADER_SIZE;
    }

    if (format == WAV_FORMAT_ALAW || format == WAV_FORMAT_MULAW) {
        self->chunk.format_chunk.bits_per_sample = 8;
    } else if (format == WAV_FORMAT_IEEE_FLOAT) {
        if (self->chunk.format_chunk.block_align != 4 && self->chunk.format_chunk.block_align != 8) {
            self->chunk.format_chunk.block_align = 4;
        }
        if (self->chunk.format_chunk.bits_per_sample > 8 * self->chunk.format_chunk.block_align) {
            self->chunk.format_chunk.bits_per_sample = 8 * self->chunk.format_chunk.block_align;
        }
    }

    wav_write_header(self);
}

void wav_set_num_channels(WavFile* self, WavU16 n_channels)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (n_channels < 1) {
        wav_err_set(WAV_ERR_PARAM, "Invalid number of channels: %u", n_channels);
        return;
    }

    self->chunk.format_chunk.n_channels = n_channels;

    self->chunk.format_chunk.avg_bytes_per_sec =
        self->chunk.format_chunk.block_align *
        self->chunk.format_chunk.sample_rate;

    wav_write_header(self);
}

void wav_set_sample_rate(WavFile* self, WavU32 sample_rate)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    self->chunk.format_chunk.sample_rate = sample_rate;

    self->chunk.format_chunk.avg_bytes_per_sec =
        self->chunk.format_chunk.block_align *
        self->chunk.format_chunk.sample_rate;

    wav_write_header(self);
}

void wav_set_valid_bits_per_sample(WavFile* self, WavU16 bits)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (bits < 1 || bits > 8 * self->chunk.format_chunk.block_align / self->chunk.format_chunk.n_channels) {
        wav_err_set(WAV_ERR_PARAM, "Invalid ValidBitsPerSample: %u", bits);
        return;
    }

    if ((self->chunk.format_chunk.format_tag == WAV_FORMAT_ALAW ||
         self->chunk.format_chunk.format_tag == WAV_FORMAT_MULAW) &&
        bits != 8) {
        wav_err_set(WAV_ERR_PARAM, "Invalid ValidBitsPerSample: %u", bits);
        return;
    }

    if (self->chunk.format_chunk.format_tag != WAV_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.bits_per_sample = bits;
    } else {
        self->chunk.format_chunk.bits_per_sample       = 8 * self->chunk.format_chunk.block_align / self->chunk.format_chunk.n_channels;
        self->chunk.format_chunk.valid_bits_per_sample = bits;
    }

    wav_write_header(self);
}

void wav_set_sample_size(WavFile* self, size_t sample_size)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (sample_size < 1) {
        wav_err_set(WAV_ERR_PARAM, "Invalid sample size: %zu", sample_size);
        return;
    }

    self->chunk.format_chunk.block_align       = (WavU16)(sample_size * self->chunk.format_chunk.n_channels);
    self->chunk.format_chunk.avg_bytes_per_sec = self->chunk.format_chunk.block_align * self->chunk.format_chunk.sample_rate;
    self->chunk.format_chunk.bits_per_sample   = (WavU16)(sample_size * 8);
    if (self->chunk.format_chunk.format_tag == WAV_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.valid_bits_per_sample = (WavU16)(sample_size * 8);
    }

    wav_write_header(self);
}

void wav_set_channel_mask(WavFile* self, WavU32 channel_mask)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (self->chunk.format_chunk.format_tag != WAV_FORMAT_EXTENSIBLE) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Extensible format is not supported");
        return;
    }

    self->chunk.format_chunk.channel_mask = channel_mask;

    wav_write_header(self);
}

void wav_set_sub_format(WavFile* self, WavU16 sub_format)
{
    if (self->mode[0] == 'r') {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (self->chunk.format_chunk.format_tag != WAV_FORMAT_EXTENSIBLE) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Extensible format is not supported");
        return;
    }

    *((WavU16*)&self->chunk.format_chunk.sub_format) = sub_format;

    wav_write_header(self);
}

WavU16 wav_get_format(WAV_CONST WavFile* self)
{
    return self->chunk.format_chunk.format_tag;
}

WavU16 wav_get_num_channels(WAV_CONST WavFile* self)
{
    return self->chunk.format_chunk.n_channels;
}

WavU32 wav_get_sample_rate(WAV_CONST WavFile* self)
{
    return self->chunk.format_chunk.sample_rate;
}

WavU16 wav_get_valid_bits_per_sample(WAV_CONST WavFile* self)
{
    if (self->chunk.format_chunk.format_tag != WAV_FORMAT_EXTENSIBLE) {
        return self->chunk.format_chunk.bits_per_sample;
    } else {
        return self->chunk.format_chunk.valid_bits_per_sample;
    }
}

size_t wav_get_sample_size(WAV_CONST WavFile* self)
{
    return self->chunk.format_chunk.block_align / self->chunk.format_chunk.n_channels;
}

size_t wav_get_length(WAV_CONST WavFile* self)
{
    return self->chunk.data_chunk.size / (self->chunk.format_chunk.block_align);
}

WavU32 wav_get_channel_mask(WAV_CONST WavFile* self)
{
    return self->chunk.format_chunk.channel_mask;
}

WavU16 wav_get_sub_format(WAV_CONST WavFile* self)
{
    return *((WavU16*)&self->chunk.format_chunk.sub_format);
}
