#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wav.h"

#if defined(__x86_64) || defined(__amd64) || defined(__i386__) || defined(__x86_64__) || defined(__LITTLE_ENDIAN__) || defined(CORE_CM7)
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

WAV_THREAD_LOCAL WavErr g_err = {WAV_OK, (char*)"", 1};

static void* wav_default_malloc(void *context, size_t size)
{
    (void)context;
    void *p = malloc(size);
    assert(p != NULL);
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
    &wav_default_malloc,
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

void* wav_malloc(size_t size)
{
    return g_alloc_funcs->malloc(g_alloc_context, size);
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
    void *new = wav_malloc(len);
    if (new == NULL)
        return NULL;

    return memcpy(new, str, len);
}

char* wav_strndup(WAV_CONST char *str, size_t n)
{
    char *result = wav_malloc(n + 1);
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

    *str = wav_malloc((size_t)size + 1);
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
        g_err.message = (char*)"";
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

typedef struct {
    WavU32 id;
    WavU32 size;
} WavChunkHeader;

typedef struct {
    WavChunkHeader header;

    WavU64 offset;

    struct {
        WavU16 format_tag;
        WavU16 num_channels;
        WavU32 sample_rate;
        WavU32 avg_bytes_per_sec;
        WavU16 block_align;
        WavU16 bits_per_sample;

        WavU16 ext_size;
        WavU16 valid_bits_per_sample;
        WavU32 channel_mask;

        WavU8 sub_format[16];
    } body;
} WavFormatChunk;

typedef struct {
    WavChunkHeader header;

    WavU64 offset;

    struct {
        WavU32 sample_length;
    } body;
} WavFactChunk;

typedef struct {
    WavChunkHeader header;
    WavU64 offset;
} WavDataChunk;

typedef struct {
    WavU32 id;
    WavU32 size;
    WavU32 wave_id;
    WavU64 offset;
} WavMasterChunk;

#pragma pack(pop)

#define WAV_CHUNK_MASTER    ((WavU32)1)
#define WAV_CHUNK_FORMAT    ((WavU32)2)
#define WAV_CHUNK_FACT      ((WavU32)4)
#define WAV_CHUNK_DATA      ((WavU32)8)

struct _WavFile {
    FILE*               fp;
    char*               filename;
    WavU32              mode;
    WavBool             is_a_new_file;

    WavMasterChunk      riff_chunk;
    WavFormatChunk      format_chunk;
    WavFactChunk        fact_chunk;
    WavDataChunk        data_chunk;
};

static WAV_CONST WavU8 default_sub_format[16] = {
    0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

void wav_parse_header(WavFile* self)
{
    size_t read_count;

    read_count = fread(&self->riff_chunk, sizeof(WavChunkHeader), 1, self->fp);
    if (read_count != 1) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Unexpected EOF");
        return;
    }

    if (self->riff_chunk.id != WAV_RIFF_CHUNK_ID) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Not a RIFF file");
        return;
    }

    read_count = fread(&self->riff_chunk.wave_id, 4, 1, self->fp);
    if (read_count != 1) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Unexpected EOF");
        return;
    }
    if (self->riff_chunk.wave_id != WAV_WAVE_ID) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Not a WAVE file");
        return;
    }

    self->riff_chunk.offset = (WavU64)ftell(self->fp);

    while (self->data_chunk.header.id != WAV_DATA_CHUNK_ID) {
        WavChunkHeader header;

        read_count = fread(&header, sizeof(WavChunkHeader), 1, self->fp);
        if (read_count != 1) {
            wav_err_set_literal(WAV_ERR_FORMAT, "Unexpected EOF");
            return;
        }

        switch (header.id) {
            case WAV_FORMAT_CHUNK_ID:
                self->format_chunk.header = header;
                self->format_chunk.offset = (WavU64)ftell(self->fp);
                read_count = fread(&self->format_chunk.body, header.size, 1, self->fp);
                if (read_count != 1) {
                    wav_err_set_literal(WAV_ERR_FORMAT, "Unexpected EOF");
                    return;
                }
                if (self->format_chunk.body.format_tag != WAV_FORMAT_PCM &&
                    self->format_chunk.body.format_tag != WAV_FORMAT_IEEE_FLOAT &&
                    self->format_chunk.body.format_tag != WAV_FORMAT_ALAW &&
                    self->format_chunk.body.format_tag != WAV_FORMAT_MULAW)
                {
                    wav_err_set(WAV_ERR_FORMAT, "Unsupported format tag: %#010x", self->format_chunk.body.format_tag);
                    return;
                }
                break;
            case WAV_FACT_CHUNK_ID:
                self->fact_chunk.header = header;
                self->fact_chunk.offset = (WavU64)ftell(self->fp);
                read_count = fread(&self->fact_chunk.body, header.size, 1, self->fp);
                if (read_count != 1) {
                    wav_err_set(WAV_ERR_FORMAT, "Unexpected EOF");
                }
                break;
            case WAV_DATA_CHUNK_ID:
                self->data_chunk.header = header;
                self->data_chunk.offset = (WavU64)ftell(self->fp);
                break;
            default:
                if (fseek(self->fp, header.size, SEEK_CUR) < 0) {
                    wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
                    return;
                }
                break;
        }
    }
}

void wav_write_header(WavFile* self)
{
    self->riff_chunk.size =
        sizeof(self->riff_chunk.wave_id) +
        (self->format_chunk.header.id == WAV_FORMAT_CHUNK_ID ? (sizeof(WavChunkHeader) + self->format_chunk.header.size) : 0) +
        (self->fact_chunk.header.id == WAV_FACT_CHUNK_ID ? (sizeof(WavChunkHeader) + self->fact_chunk.header.size) : 0) +
        (self->data_chunk.header.id == WAV_DATA_CHUNK_ID ? (sizeof(WavChunkHeader) + self->data_chunk.header.size) : 0);

    if (fseek(self->fp, 0, SEEK_SET) != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fwrite(&self->riff_chunk, sizeof(WavChunkHeader) + 4, 1, self->fp) != 1) {
        wav_err_set(WAV_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
        return;
    }

    if (self->format_chunk.header.id == WAV_FORMAT_CHUNK_ID) {
        if (fseek(self->fp, (long)(self->format_chunk.offset - sizeof(WavChunkHeader)), SEEK_SET) != 0) {
            wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->format_chunk.header, sizeof(WavChunkHeader), 1, self->fp) != 1) {
            wav_err_set(WAV_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
        if (fwrite(&self->format_chunk.body, self->format_chunk.header.size, 1, self->fp) != 1) {
            wav_err_set(WAV_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
    }

    if (self->fact_chunk.header.id == WAV_FACT_CHUNK_ID) {
        if (fseek(self->fp, (long)(self->fact_chunk.offset - sizeof(WavChunkHeader)), SEEK_SET) != 0) {
            wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->fact_chunk.header, sizeof(WavChunkHeader), 1, self->fp) != 1) {
            wav_err_set(WAV_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
        if (fwrite(&self->fact_chunk.body, self->fact_chunk.header.size, 1, self->fp) != 1) {
            wav_err_set(WAV_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
    }

    if (self->data_chunk.header.id == WAV_DATA_CHUNK_ID) {
        if (fseek(self->fp, (long)(self->data_chunk.offset - sizeof(WavChunkHeader)), SEEK_SET) != 0) {
            wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->data_chunk.header, sizeof(WavChunkHeader), 1, self->fp) != 1) {
            wav_err_set(WAV_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
    }
}

void wav_init(WavFile* self, WAV_CONST char* filename, WavU32 mode)
{
    memset(self, 0, sizeof(WavFile));

    if (mode & WAV_OPEN_READ) {
        if ((mode & WAV_OPEN_WRITE) || (mode & WAV_OPEN_APPEND)) {
            self->fp = fopen(filename, "wb+");
        } else {
            self->fp = fopen(filename, "rb");
        }
    } else {
        if ((mode & WAV_OPEN_WRITE) || (mode & WAV_OPEN_APPEND)) {
            self->fp = fopen(filename, "wb+");
        } else {
            wav_err_set_literal(WAV_ERR_PARAM, "Invalid mode");
            return;
        }
    }

    if (self->fp == NULL) {
        wav_err_set(WAV_ERR_OS, "Error when opening %s [errno %d: %s]", filename, errno, strerror(errno));
        return;
    }

    self->filename = wav_strdup(filename);
    self->mode = mode;

    if (!(self->mode & WAV_OPEN_WRITE) && !(self->mode & WAV_OPEN_APPEND)) {
        wav_parse_header(self);
        return;
    }

    if (self->mode & WAV_OPEN_APPEND) {
        wav_parse_header(self);
        if (g_err.code == WAV_OK) {
            // If the header parsing was successful, return immediately.
            return;
        } else {
            // Header parsing failed. Regard it as a new file.
            wav_err_clear();
            rewind(self->fp);
            self->is_a_new_file = WAV_TRUE;
        }
    }

    // reaches here only if creating a new file

    self->riff_chunk.id = WAV_RIFF_CHUNK_ID;
    /* self->chunk.size = calculated by wav_write_header */
    self->riff_chunk.wave_id = WAV_WAVE_ID;
    self->riff_chunk.offset = sizeof(WavChunkHeader) + 4;

    self->format_chunk.header.id                = WAV_FORMAT_CHUNK_ID;
    self->format_chunk.header.size              = (WavU32)((WavUIntPtr)&self->format_chunk.body.ext_size - (WavUIntPtr)&self->format_chunk.body);
    self->format_chunk.offset                   = self->riff_chunk.offset + sizeof(WavChunkHeader);
    self->format_chunk.body.format_tag          = WAV_FORMAT_PCM;
    self->format_chunk.body.num_channels        = 2;
    self->format_chunk.body.sample_rate         = 44100;
    self->format_chunk.body.avg_bytes_per_sec   = 44100 * 2 * 2;
    self->format_chunk.body.block_align         = 4;
    self->format_chunk.body.bits_per_sample     = 16;

    memcpy(self->format_chunk.body.sub_format, default_sub_format, 16);

    self->data_chunk.header.id = WAV_DATA_CHUNK_ID;
    self->data_chunk.offset = self->format_chunk.offset + self->format_chunk.header.size + sizeof(WavChunkHeader);

    wav_write_header(self);
}

void wav_finalize(WavFile* self)
{
    int ret;

    wav_free(self->filename);

    if (self->fp == NULL) {
        return;
    }

    ret = fclose(self->fp);
    if (ret != 0) {
        fprintf(stderr, "[WARN] [libwav] fclose failed with code %d [errno %d: %s]", ret, errno, strerror(errno));
        return;
    }
}

WavFile* wav_open(WAV_CONST char* filename, WavU32 mode)
{
    WavFile* self = wav_malloc(sizeof(WavFile));
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

WavFile* wav_reopen(WavFile* self, WAV_CONST char* filename, WavU32 mode)
{
    wav_finalize(self);
    wav_init(self, filename, mode);
    return self;
}

size_t wav_read(WavFile* self, void *buffer, size_t count)
{
    size_t read_count;
    WavU16 n_channels = wav_get_num_channels(self);
    size_t sample_size = wav_get_sample_size(self);
    size_t len_remain;

    if (!(self->mode & WAV_OPEN_READ)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not readable");
        return 0;
    }

    if (self->format_chunk.body.format_tag == WAV_FORMAT_EXTENSIBLE) {
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
        wav_err_set(WAV_ERR_OS, "Error when reading %s [errno %d: %s]", self->filename, errno, strerror(errno));
        return 0;
    }

    return read_count / n_channels;
}

WAV_INLINE void wav_update_sizes(WavFile *self)
{
    long int save_pos = ftell(self->fp);
    if (fseek(self->fp, (long)(sizeof(WavChunkHeader) - 4), SEEK_SET) != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fwrite(&self->riff_chunk.size, 4, 1, self->fp) != 1) {
        wav_err_set(WAV_ERR_OS, "fwrite() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (self->fact_chunk.header.id == WAV_FACT_CHUNK_ID) {
        if (fseek(self->fp, (long)self->fact_chunk.offset, SEEK_SET) != 0) {
            wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->fact_chunk.body.sample_length, 4, 1, self->fp) != 1) {
            wav_err_set(WAV_ERR_OS, "fwrite() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
    }
    if (fseek(self->fp, (long)(self->data_chunk.offset - 4), SEEK_SET) != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fwrite(&self->data_chunk.header.size, 4, 1, self->fp) != 1) {
        wav_err_set(WAV_ERR_OS, "fwrite() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fseek(self->fp, save_pos, SEEK_SET) != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
}

size_t wav_write(WavFile* self, WAV_CONST void *buffer, size_t count)
{
    size_t write_count;
    WavU16 n_channels = wav_get_num_channels(self);
    size_t sample_size = wav_get_sample_size(self);

    if (!(self->mode & WAV_OPEN_WRITE) && !(self->mode & WAV_OPEN_APPEND)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return 0;
    }

    if (self->format_chunk.body.format_tag == WAV_FORMAT_EXTENSIBLE) {
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

    if (!(self->mode & WAV_OPEN_READ) && !(self->mode & WAV_OPEN_WRITE)) {
        wav_seek(self, 0, SEEK_END);
        if (g_err.code != WAV_OK) {
            return 0;
        }
    }

    write_count = fwrite(buffer, sample_size, n_channels * count, self->fp);
    if (ferror(self->fp)) {
        wav_err_set(WAV_ERR_OS, "Error when writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
        return 0;
    }

    self->riff_chunk.size += write_count * sample_size;
    if (self->fact_chunk.header.id == WAV_FACT_CHUNK_ID) {
        self->fact_chunk.body.sample_length += write_count / n_channels;
    }
    self->data_chunk.header.size += write_count * sample_size;

    wav_update_sizes(self);
    if (g_err.code != WAV_OK)
        return 0;

    return write_count / n_channels;
}

long int wav_tell(WAV_CONST WavFile* self)
{
    long pos = ftell(self->fp);

    if (pos == -1L) {
        wav_err_set(WAV_ERR_OS, "ftell() failed [errno %d: %s]", errno, strerror(errno));
        return -1L;
    }

    assert(pos >= (long)self->data_chunk.offset);

    return (long)(((WavU64)pos - self->data_chunk.offset) / (self->format_chunk.body.block_align));
}

int wav_seek(WavFile* self, long int offset, int origin)
{
    size_t length = wav_get_length(self);
    int    ret;

    if (origin == SEEK_CUR) {
        offset += (long)wav_tell(self);
    } else if (origin == SEEK_END) {
        offset += (long)length;
    }

    /* POSIX allows seeking beyond end of file */
    if (offset >= 0) {
        offset *= self->format_chunk.body.block_align;
    } else {
        wav_err_set_literal(WAV_ERR_PARAM, "Invalid seek");
        return (int)g_err.code;
    }

    ret = fseek(self->fp, (long)self->data_chunk.offset + offset, SEEK_SET);

    if (ret != 0) {
        wav_err_set(WAV_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return (int)ret;
    }

    return 0;
}

void wav_rewind(WavFile* self)
{
    wav_seek(self, 0, SEEK_SET);
}

int wav_eof(WAV_CONST WavFile* self)
{
    return feof(self->fp) || ftell(self->fp) == (long)(self->data_chunk.offset + self->data_chunk.header.size);
}

int wav_flush(WavFile* self)
{
    int ret = fflush(self->fp);

    if (ret != 0) {
        wav_err_set(WAV_ERR_OS, "fflush() failed [errno %d: %s]", errno, strerror(errno));
    }

    return ret;
}

void wav_set_format(WavFile* self, WavU16 format)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (format == self->format_chunk.body.format_tag)
        return;

    self->format_chunk.body.format_tag = format;
    if (format != WAV_FORMAT_PCM && format != WAV_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.ext_size = 0;
        self->format_chunk.header.size = (WavU32)((WavUIntPtr)&self->format_chunk.body.ext_size - (WavUIntPtr)&self->format_chunk.body);
    } else if (format == WAV_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.ext_size = 22;
        self->format_chunk.header.size = sizeof(WavFormatChunk) - sizeof(WavChunkHeader);
    }

    if (format == WAV_FORMAT_ALAW || format == WAV_FORMAT_MULAW) {
        WavU16 sample_size = wav_get_sample_size(self);
        if (sample_size != 1) {
            wav_set_sample_size(self, 1);
        }
    } else if (format == WAV_FORMAT_IEEE_FLOAT) {
        WavU16 sample_size = wav_get_sample_size(self);
        if (sample_size != 4 && sample_size != 8) {
            wav_set_sample_size(self, 4);
        }
    }

    wav_write_header(self);
}

void wav_set_num_channels(WavFile* self, WavU16 num_channels)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (num_channels < 1) {
        wav_err_set(WAV_ERR_PARAM, "Invalid number of channels: %u", num_channels);
        return;
    }

    WavU16 old_num_channels = self->format_chunk.body.num_channels;
    if (num_channels == old_num_channels)
        return;

    self->format_chunk.body.num_channels = num_channels;
    self->format_chunk.body.block_align = self->format_chunk.body.block_align / old_num_channels * num_channels;
    self->format_chunk.body.avg_bytes_per_sec = self->format_chunk.body.block_align * self->format_chunk.body.sample_rate;

    wav_write_header(self);
}

void wav_set_sample_rate(WavFile* self, WavU32 sample_rate)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (sample_rate == self->format_chunk.body.sample_rate)
        return;

    self->format_chunk.body.sample_rate = sample_rate;
    self->format_chunk.body.avg_bytes_per_sec = self->format_chunk.body.block_align * self->format_chunk.body.sample_rate;

    wav_write_header(self);
}

void wav_set_valid_bits_per_sample(WavFile* self, WavU16 bits)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (bits < 1 || bits > 8 * self->format_chunk.body.block_align / self->format_chunk.body.num_channels) {
        wav_err_set(WAV_ERR_PARAM, "Invalid ValidBitsPerSample: %u", bits);
        return;
    }

    if ((self->format_chunk.body.format_tag == WAV_FORMAT_ALAW || self->format_chunk.body.format_tag == WAV_FORMAT_MULAW) && bits != 8) {
        wav_err_set(WAV_ERR_PARAM, "Invalid ValidBitsPerSample: %u", bits);
        return;
    }

    if (self->format_chunk.body.format_tag != WAV_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.bits_per_sample = bits;
    } else {
        self->format_chunk.body.bits_per_sample = 8 * self->format_chunk.body.block_align / self->format_chunk.body.num_channels;
        self->format_chunk.body.valid_bits_per_sample = bits;
    }

    wav_write_header(self);
}

void wav_set_sample_size(WavFile* self, size_t sample_size)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (sample_size < 1) {
        wav_err_set(WAV_ERR_PARAM, "Invalid sample size: %zu", sample_size);
        return;
    }

    self->format_chunk.body.block_align = (WavU16)(sample_size * self->format_chunk.body.num_channels);
    self->format_chunk.body.avg_bytes_per_sec = self->format_chunk.body.block_align * self->format_chunk.body.sample_rate;
    self->format_chunk.body.bits_per_sample = (WavU16)(sample_size * 8);
    if (self->format_chunk.body.format_tag == WAV_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.valid_bits_per_sample = (WavU16)(sample_size * 8);
    }

    wav_write_header(self);
}

void wav_set_channel_mask(WavFile* self, WavU32 channel_mask)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (self->format_chunk.body.format_tag != WAV_FORMAT_EXTENSIBLE) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Extensible format is not supported");
        return;
    }

    self->format_chunk.body.channel_mask = channel_mask;

    wav_write_header(self);
}

void wav_set_sub_format(WavFile* self, WavU16 sub_format)
{
    if (!(self->mode & WAV_OPEN_WRITE) && !((self->mode & WAV_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wav_err_set_literal(WAV_ERR_MODE, "This WavFile is not writable");
        return;
    }

    if (self->format_chunk.body.format_tag != WAV_FORMAT_EXTENSIBLE) {
        wav_err_set_literal(WAV_ERR_FORMAT, "Extensible format is not supported");
        return;
    }

    self->format_chunk.body.sub_format[0] = (WavU8)(sub_format & 0xff);
    self->format_chunk.body.sub_format[1] = (WavU8)(sub_format >> 8);

    wav_write_header(self);
}

WavU16 wav_get_format(WAV_CONST WavFile* self)
{
    return self->format_chunk.body.format_tag;
}

WavU16 wav_get_num_channels(WAV_CONST WavFile* self)
{
    return self->format_chunk.body.num_channels;
}

WavU32 wav_get_sample_rate(WAV_CONST WavFile* self)
{
    return self->format_chunk.body.sample_rate;
}

WavU16 wav_get_valid_bits_per_sample(WAV_CONST WavFile* self)
{
    if (self->format_chunk.body.format_tag != WAV_FORMAT_EXTENSIBLE) {
        return self->format_chunk.body.bits_per_sample;
    } else {
        return self->format_chunk.body.valid_bits_per_sample;
    }
}

size_t wav_get_sample_size(WAV_CONST WavFile* self)
{
    return self->format_chunk.body.block_align / self->format_chunk.body.num_channels;
}

size_t wav_get_length(WAV_CONST WavFile* self)
{
    return self->data_chunk.header.size / (self->format_chunk.body.block_align);
}

WavU32 wav_get_channel_mask(WAV_CONST WavFile* self)
{
    return self->format_chunk.body.channel_mask;
}

WavU16 wav_get_sub_format(WAV_CONST WavFile* self)
{
    WavU16 sub_format = self->format_chunk.body.sub_format[1];
    sub_format <<= 8;
    sub_format |= self->format_chunk.body.sub_format[0];
    return sub_format;
}
