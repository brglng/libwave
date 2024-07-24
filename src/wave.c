#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wave.h"

#define WAVE_ENDIAN_ORDER_LITTLE    0x41424344UL
#define WAVE_ENDIAN_ORDER_BIG       0x44434241UL
#define WAVE_ENDIAN_ORDER_PDP       0x42414443UL
#define WAVE_ENDIAN_ORDER           'ABCD'

#if WAVE_ENDIAN_ORDER == WAVE_ENDIAN_ORDER_LITTLE
#define WAVE_ENDIAN_LITTLE 1
#define WAVE_ENDIAN_BIG 0
#elif WAVE_ENDIAN_ORDER == WAVE_ENDIAN_ORDER_BIG
#define WAVE_ENDIAN_LITTLE 0
#define WAVE_ENDIAN_BIG 1
#else
#error "unsupported endianess"
#endif

#if WAVE_ENDIAN_LITTLE
#define WAVE_RIFF_CHUNK_ID       ((WaveU32)'FFIR')
#define WAVE_FORMAT_CHUNK_ID     ((WaveU32)' tmf')
#define WAVE_FACT_CHUNK_ID       ((WaveU32)'tcaf')
#define WAVE_DATA_CHUNK_ID       ((WaveU32)'atad')
#define WAVE_WAVE_ID             ((WaveU32)'EVAW')
#endif

#if WAVE_ENDIAN_BIG
#define WAVE_RIFF_CHUNK_ID       ((WaveU32)'RIFF')
#define WAVE_FORMAT_CHUNK_ID     ((WaveU32)'fmt ')
#define WAVE_FACT_CHUNK_ID       ((WaveU32)'fact')
#define WAVE_DATA_CHUNK_ID       ((WaveU32)'data')
#define WAVE_WAVE_ID             ((WaveU32)'WAVE')
#endif

WAVE_THREAD_LOCAL WaveErr g_err = {WAVE_OK, (char*)"", 1};

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void* wave_default_malloc(void *context, size_t size)
{
    (void)context;
    void *p = malloc(size);
    assert(p != NULL);
    return p;
}

static void* wave_default_realloc(void *context, void *p, size_t size)
{
    (void)context;
    void *ptr = realloc(p, size);
    assert(ptr != NULL);
    return ptr;
}

static void wave_default_free(void *context, void *p)
{
    (void)context;
    free(p);
}

static WaveAllocFuncs g_default_alloc_funcs = {
    &wave_default_malloc,
    &wave_default_realloc,
    &wave_default_free
};

static void *g_alloc_context = NULL;
static WAVE_CONST WaveAllocFuncs* g_alloc_funcs = &g_default_alloc_funcs;

void wave_set_allocator(void *context, WAVE_CONST WaveAllocFuncs *funcs)
{
    g_alloc_context = context;
    g_alloc_funcs = funcs;
}

void* wave_malloc(size_t size)
{
    return g_alloc_funcs->malloc(g_alloc_context, size);
}

void* wave_realloc(void *p, size_t size)
{
    return g_alloc_funcs->realloc(g_alloc_context, p, size);
}

void wave_free(void *p)
{
    if (p != NULL) {
        g_alloc_funcs->free(g_alloc_context, p);
    }
}

char* wave_strdup(WAVE_CONST char *str)
{
    size_t len = strlen(str) + 1;
    void *new = wave_malloc(len);
    if (new == NULL)
        return NULL;

    return memcpy(new, str, len);
}

char* wave_strndup(WAVE_CONST char *str, size_t n)
{
    char *result = wave_malloc(n + 1);
    if (result == NULL)
        return NULL;

    result[n] = 0;
    return memcpy(result, str, n);
}

int wave_vasprintf(char **str, WAVE_CONST char *format, va_list args)
{
    int size = 0;

    va_list args_copy;
    va_copy(args_copy, args);
    size = vsnprintf(NULL, (size_t)size, format, args_copy);
    va_end(args_copy);

    if (size < 0) {
        return size;
    }

    *str = wave_malloc((size_t)size + 1);
    if (*str == NULL)
        return -1;

    return vsprintf(*str, format, args);
}

int wave_asprintf(char **str, WAVE_CONST char *format, ...)
{
    va_list args;
    va_start(args, format);
    int size = wave_vasprintf(str, format, args);
    va_end(args);
    return size;
}

WAVE_CONST WaveErr* wave_err(void)
{
    return &g_err;
}

void wave_err_clear(void)
{
    if (g_err.code != WAVE_OK) {
        if (!g_err._is_literal) {
            wave_free(g_err.message);
        }
        g_err.code = WAVE_OK;
        g_err.message = (char*)"";
        g_err._is_literal = 1;
    }
}

WAVE_INLINE void wave_err_set(WaveErrCode code, WAVE_CONST char *format, ...)
{
    assert(g_err.code == WAVE_OK);
    va_list args;
    va_start(args, format);
    g_err.code = code;
    wave_vasprintf(&g_err.message, format, args);
    g_err._is_literal = 0;
    va_end(args);
}

WAVE_INLINE void wave_err_set_literal(WaveErrCode code, WAVE_CONST char *message)
{
    assert(g_err.code == WAVE_OK);
    g_err.code = code;
    g_err.message = (char *)message;
    g_err._is_literal = 1;
}

#pragma pack(push, 1)

typedef struct {
    WaveU32 id;
    WaveU32 size;
} WaveChunkHeader;

typedef struct {
    WaveChunkHeader header;

    WaveU64 offset;

    struct {
        WaveU16 format_tag;
        WaveU16 num_channels;
        WaveU32 sample_rate;
        WaveU32 avg_bytes_per_sec;
        WaveU16 block_align;
        WaveU16 bits_per_sample;

        WaveU16 ext_size;
        WaveU16 valid_bits_per_sample;
        WaveU32 channel_mask;

        WaveU8 sub_format[16];
    } body;
} WaveFormatChunk;

typedef struct {
    WaveChunkHeader header;

    WaveU64 offset;

    struct {
        WaveU32 sample_length;
    } body;
} WaveFactChunk;

typedef struct {
    WaveChunkHeader header;
    WaveU64 offset;
} WaveDataChunk;

typedef struct {
    WaveU32 id;
    WaveU32 size;
    WaveU32 wave_id;
    WaveU64 offset;
} WaveMasterChunk;

#pragma pack(pop)

#define WAVE_CHUNK_MASTER    ((WaveU32)1)
#define WAVE_CHUNK_FORMAT    ((WaveU32)2)
#define WAVE_CHUNK_FACT      ((WaveU32)4)
#define WAVE_CHUNK_DATA      ((WaveU32)8)

struct _WaveFile {
    FILE*               fp;
    char*               filename;
    WaveU32              mode;
    WaveBool             is_a_new_file;

    WaveMasterChunk      riff_chunk;
    WaveFormatChunk      format_chunk;
    WaveFactChunk        fact_chunk;
    WaveDataChunk        data_chunk;
};

static WAVE_CONST WaveU8 default_sub_format[16] = {
    0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

void wave_parse_header(WaveFile* self)
{
    size_t read_count;

    read_count = fread(&self->riff_chunk, sizeof(WaveChunkHeader), 1, self->fp);
    if (read_count != 1) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Unexpected EOF");
        return;
    }

    if (self->riff_chunk.id != WAVE_RIFF_CHUNK_ID) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Not a RIFF file");
        return;
    }

    read_count = fread(&self->riff_chunk.wave_id, 4, 1, self->fp);
    if (read_count != 1) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Unexpected EOF");
        return;
    }
    if (self->riff_chunk.wave_id != WAVE_WAVE_ID) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Not a WAVE file");
        return;
    }

    self->riff_chunk.offset = (WaveU64)ftell(self->fp);

    while (self->data_chunk.header.id != WAVE_DATA_CHUNK_ID) {
        WaveChunkHeader header;

        read_count = fread(&header, sizeof(WaveChunkHeader), 1, self->fp);
        if (read_count != 1) {
            wave_err_set_literal(WAVE_ERR_FORMAT, "Unexpected EOF");
            return;
        }

        switch (header.id) {
            case WAVE_FORMAT_CHUNK_ID:
                self->format_chunk.header = header;
                self->format_chunk.offset = (WaveU64)ftell(self->fp);
                read_count = fread(&self->format_chunk.body, MIN(header.size, sizeof(self->format_chunk.body)), 1, self->fp);
                if (read_count != 1) {
                    wave_err_set_literal(WAVE_ERR_FORMAT, "Unexpected EOF");
                    return;
                }
                if (self->format_chunk.body.format_tag != WAVE_FORMAT_PCM &&
                    self->format_chunk.body.format_tag != WAVE_FORMAT_IEEE_FLOAT &&
                    self->format_chunk.body.format_tag != WAVE_FORMAT_ALAW &&
                    self->format_chunk.body.format_tag != WAVE_FORMAT_MULAW)
                {
                    wave_err_set(WAVE_ERR_FORMAT, "Unsupported format tag: %#010x", self->format_chunk.body.format_tag);
                    return;
                }
                break;
            case WAVE_FACT_CHUNK_ID:
                self->fact_chunk.header = header;
                self->fact_chunk.offset = (WaveU64)ftell(self->fp);
                read_count = fread(&self->fact_chunk.body, MIN(header.size, sizeof(self->fact_chunk.body)), 1, self->fp);
                if (read_count != 1) {
                    wave_err_set(WAVE_ERR_FORMAT, "Unexpected EOF");
                }
                break;
            case WAVE_DATA_CHUNK_ID:
                self->data_chunk.header = header;
                self->data_chunk.offset = (WaveU64)ftell(self->fp);
                break;
            default:
                if (fseek(self->fp, header.size, SEEK_CUR) < 0) {
                    wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
                    return;
                }
                break;
        }
    }
}

void wave_write_header(WaveFile* self)
{
    self->riff_chunk.size =
        sizeof(self->riff_chunk.wave_id) +
        (self->format_chunk.header.id == WAVE_FORMAT_CHUNK_ID ? (sizeof(WaveChunkHeader) + self->format_chunk.header.size) : 0) +
        (self->fact_chunk.header.id == WAVE_FACT_CHUNK_ID ? (sizeof(WaveChunkHeader) + self->fact_chunk.header.size) : 0) +
        (self->data_chunk.header.id == WAVE_DATA_CHUNK_ID ? (sizeof(WaveChunkHeader) + self->data_chunk.header.size) : 0);

    if (fseek(self->fp, 0, SEEK_SET) != 0) {
        wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fwrite(&self->riff_chunk, sizeof(WaveChunkHeader) + 4, 1, self->fp) != 1) {
        wave_err_set(WAVE_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
        return;
    }

    if (self->format_chunk.header.id == WAVE_FORMAT_CHUNK_ID) {
        if (fseek(self->fp, (long)(self->format_chunk.offset - sizeof(WaveChunkHeader)), SEEK_SET) != 0) {
            wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->format_chunk.header, sizeof(WaveChunkHeader), 1, self->fp) != 1) {
            wave_err_set(WAVE_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
        if (fwrite(&self->format_chunk.body, self->format_chunk.header.size, 1, self->fp) != 1) {
            wave_err_set(WAVE_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
    }

    if (self->fact_chunk.header.id == WAVE_FACT_CHUNK_ID) {
        if (fseek(self->fp, (long)(self->fact_chunk.offset - sizeof(WaveChunkHeader)), SEEK_SET) != 0) {
            wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->fact_chunk.header, sizeof(WaveChunkHeader), 1, self->fp) != 1) {
            wave_err_set(WAVE_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
        if (fwrite(&self->fact_chunk.body, self->fact_chunk.header.size, 1, self->fp) != 1) {
            wave_err_set(WAVE_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
    }

    if (self->data_chunk.header.id == WAVE_DATA_CHUNK_ID) {
        if (fseek(self->fp, (long)(self->data_chunk.offset - sizeof(WaveChunkHeader)), SEEK_SET) != 0) {
            wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->data_chunk.header, sizeof(WaveChunkHeader), 1, self->fp) != 1) {
            wave_err_set(WAVE_ERR_OS, "Error while writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
            return;
        }
    }
}

void wave_init(WaveFile* self, WAVE_CONST char* filename, WaveU32 mode)
{
    memset(self, 0, sizeof(WaveFile));

    if (mode & WAVE_OPEN_READ) {
        if ((mode & WAVE_OPEN_WRITE) || (mode & WAVE_OPEN_APPEND)) {
            self->fp = fopen(filename, "wb+");
        } else {
            self->fp = fopen(filename, "rb");
        }
    } else {
        if ((mode & WAVE_OPEN_WRITE) || (mode & WAVE_OPEN_APPEND)) {
            self->fp = fopen(filename, "wb+");
        } else {
            wave_err_set_literal(WAVE_ERR_PARAM, "Invalid mode");
            return;
        }
    }

    if (self->fp == NULL) {
        wave_err_set(WAVE_ERR_OS, "Error when opening %s [errno %d: %s]", filename, errno, strerror(errno));
        return;
    }

    self->filename = wave_strdup(filename);
    self->mode = mode;

    if (!(self->mode & WAVE_OPEN_WRITE) && !(self->mode & WAVE_OPEN_APPEND)) {
        wave_parse_header(self);
        return;
    }

    if (self->mode & WAVE_OPEN_APPEND) {
        wave_parse_header(self);
        if (g_err.code == WAVE_OK) {
            // If the header parsing was successful, return immediately.
            return;
        } else {
            // Header parsing failed. Regard it as a new file.
            wave_err_clear();
            rewind(self->fp);
            self->is_a_new_file = WAVE_TRUE;
        }
    }

    // reaches here only if creating a new file

    self->riff_chunk.id = WAVE_RIFF_CHUNK_ID;
    /* self->chunk.size = calculated by wave_write_header */
    self->riff_chunk.wave_id = WAVE_WAVE_ID;
    self->riff_chunk.offset = sizeof(WaveChunkHeader) + 4;

    self->format_chunk.header.id                = WAVE_FORMAT_CHUNK_ID;
    self->format_chunk.header.size              = (WaveU32)((WaveUIntPtr)&self->format_chunk.body.ext_size - (WaveUIntPtr)&self->format_chunk.body);
    self->format_chunk.offset                   = self->riff_chunk.offset + sizeof(WaveChunkHeader);
    self->format_chunk.body.format_tag          = WAVE_FORMAT_PCM;
    self->format_chunk.body.num_channels        = 2;
    self->format_chunk.body.sample_rate         = 44100;
    self->format_chunk.body.avg_bytes_per_sec   = 44100 * 2 * 2;
    self->format_chunk.body.block_align         = 4;
    self->format_chunk.body.bits_per_sample     = 16;

    memcpy(self->format_chunk.body.sub_format, default_sub_format, 16);

    self->data_chunk.header.id = WAVE_DATA_CHUNK_ID;
    self->data_chunk.offset = self->format_chunk.offset + self->format_chunk.header.size + sizeof(WaveChunkHeader);

    wave_write_header(self);
}

void wave_finalize(WaveFile* self)
{
    int ret;

    wave_free(self->filename);

    if (self->fp == NULL) {
        return;
    }

    ret = fclose(self->fp);
    if (ret != 0) {
        fprintf(stderr, "[WARN] [libwav] fclose failed with code %d [errno %d: %s]", ret, errno, strerror(errno));
        return;
    }
}

WaveFile* wave_open(WAVE_CONST char* filename, WaveU32 mode)
{
    WaveFile* self = wave_malloc(sizeof(WaveFile));
    if (self == NULL) {
        return NULL;
    }

    wave_init(self, filename, mode);

    return self;
}

void wave_close(WaveFile* self)
{
    wave_finalize(self);
    wave_free(self);
}

WaveFile* wave_reopen(WaveFile* self, WAVE_CONST char* filename, WaveU32 mode)
{
    wave_finalize(self);
    wave_init(self, filename, mode);
    return self;
}

size_t wave_read(WaveFile* self, void *buffer, size_t count)
{
    size_t read_count;
    WaveU16 n_channels = wave_get_num_channels(self);
    size_t sample_size = wave_get_sample_size(self);
    size_t len_remain;

    if (!(self->mode & WAVE_OPEN_READ)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not readable");
        return 0;
    }

    if (self->format_chunk.body.format_tag == WAVE_FORMAT_EXTENSIBLE) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Extensible format is not supported");
        return 0;
    }

    len_remain = wave_get_length(self) - (size_t)wave_tell(self);
    if (g_err.code != WAVE_OK) {
        return 0;
    }
    count = (count <= len_remain) ? count : len_remain;

    if (count == 0) {
        return 0;
    }

    read_count = fread(buffer, sample_size, n_channels * count, self->fp);
    if (ferror(self->fp)) {
        wave_err_set(WAVE_ERR_OS, "Error when reading %s [errno %d: %s]", self->filename, errno, strerror(errno));
        return 0;
    }

    return read_count / n_channels;
}

WAVE_INLINE void wave_update_sizes(WaveFile *self)
{
    long int save_pos = ftell(self->fp);
    if (fseek(self->fp, (long)(sizeof(WaveChunkHeader) - 4), SEEK_SET) != 0) {
        wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fwrite(&self->riff_chunk.size, 4, 1, self->fp) != 1) {
        wave_err_set(WAVE_ERR_OS, "fwrite() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (self->fact_chunk.header.id == WAVE_FACT_CHUNK_ID) {
        if (fseek(self->fp, (long)self->fact_chunk.offset, SEEK_SET) != 0) {
            wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
        if (fwrite(&self->fact_chunk.body.sample_length, 4, 1, self->fp) != 1) {
            wave_err_set(WAVE_ERR_OS, "fwrite() failed [errno %d: %s]", errno, strerror(errno));
            return;
        }
    }
    if (fseek(self->fp, (long)(self->data_chunk.offset - 4), SEEK_SET) != 0) {
        wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fwrite(&self->data_chunk.header.size, 4, 1, self->fp) != 1) {
        wave_err_set(WAVE_ERR_OS, "fwrite() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
    if (fseek(self->fp, save_pos, SEEK_SET) != 0) {
        wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return;
    }
}

size_t wave_write(WaveFile* self, WAVE_CONST void *buffer, size_t count)
{
    size_t write_count;
    WaveU16 n_channels = wave_get_num_channels(self);
    size_t sample_size = wave_get_sample_size(self);

    if (!(self->mode & WAVE_OPEN_WRITE) && !(self->mode & WAVE_OPEN_APPEND)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return 0;
    }

    if (self->format_chunk.body.format_tag == WAVE_FORMAT_EXTENSIBLE) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Extensible format is not supported");
        return 0;
    }

    if (count == 0) {
        return 0;
    }

    wave_tell(self);
    if (g_err.code != WAVE_OK) {
        return 0;
    }

    if (!(self->mode & WAVE_OPEN_READ) && !(self->mode & WAVE_OPEN_WRITE)) {
        wave_seek(self, 0, SEEK_END);
        if (g_err.code != WAVE_OK) {
            return 0;
        }
    }

    write_count = fwrite(buffer, sample_size, n_channels * count, self->fp);
    if (ferror(self->fp)) {
        wave_err_set(WAVE_ERR_OS, "Error when writing to %s [errno %d: %s]", self->filename, errno, strerror(errno));
        return 0;
    }

    self->riff_chunk.size += write_count * sample_size;
    if (self->fact_chunk.header.id == WAVE_FACT_CHUNK_ID) {
        self->fact_chunk.body.sample_length += write_count / n_channels;
    }
    self->data_chunk.header.size += write_count * sample_size;

    wave_update_sizes(self);
    if (g_err.code != WAVE_OK)
        return 0;

    return write_count / n_channels;
}

long int wave_tell(WAVE_CONST WaveFile* self)
{
    long pos = ftell(self->fp);

    if (pos == -1L) {
        wave_err_set(WAVE_ERR_OS, "ftell() failed [errno %d: %s]", errno, strerror(errno));
        return -1L;
    }

    assert(pos >= (long)self->data_chunk.offset);

    return (long)(((WaveU64)pos - self->data_chunk.offset) / (self->format_chunk.body.block_align));
}

int wave_seek(WaveFile* self, long int offset, int origin)
{
    size_t length = wave_get_length(self);
    int    ret;

    if (origin == SEEK_CUR) {
        offset += (long)wave_tell(self);
    } else if (origin == SEEK_END) {
        offset += (long)length;
    }

    /* POSIX allows seeking beyond end of file */
    if (offset >= 0) {
        offset *= self->format_chunk.body.block_align;
    } else {
        wave_err_set_literal(WAVE_ERR_PARAM, "Invalid seek");
        return (int)g_err.code;
    }

    ret = fseek(self->fp, (long)self->data_chunk.offset + offset, SEEK_SET);

    if (ret != 0) {
        wave_err_set(WAVE_ERR_OS, "fseek() failed [errno %d: %s]", errno, strerror(errno));
        return (int)ret;
    }

    return 0;
}

void wave_rewind(WaveFile* self)
{
    wave_seek(self, 0, SEEK_SET);
}

int wave_eof(WAVE_CONST WaveFile* self)
{
    return feof(self->fp) || ftell(self->fp) == (long)(self->data_chunk.offset + self->data_chunk.header.size);
}

int wave_flush(WaveFile* self)
{
    int ret = fflush(self->fp);

    if (ret != 0) {
        wave_err_set(WAVE_ERR_OS, "fflush() failed [errno %d: %s]", errno, strerror(errno));
    }

    return ret;
}

void wave_set_format(WaveFile* self, WaveU16 format)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (format == self->format_chunk.body.format_tag)
        return;

    self->format_chunk.body.format_tag = format;
    if (format != WAVE_FORMAT_PCM && format != WAVE_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.ext_size = 0;
        self->format_chunk.header.size = (WaveU32)((WaveUIntPtr)&self->format_chunk.body.ext_size - (WaveUIntPtr)&self->format_chunk.body);
    } else if (format == WAVE_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.ext_size = 22;
        self->format_chunk.header.size = sizeof(WaveFormatChunk) - sizeof(WaveChunkHeader);
    }

    if (format == WAVE_FORMAT_ALAW || format == WAVE_FORMAT_MULAW) {
        WaveU16 sample_size = wave_get_sample_size(self);
        if (sample_size != 1) {
            wave_set_sample_size(self, 1);
        }
    } else if (format == WAVE_FORMAT_IEEE_FLOAT) {
        WaveU16 sample_size = wave_get_sample_size(self);
        if (sample_size != 4 && sample_size != 8) {
            wave_set_sample_size(self, 4);
        }
    }

    wave_write_header(self);
}

void wave_set_num_channels(WaveFile* self, WaveU16 num_channels)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (num_channels < 1) {
        wave_err_set(WAVE_ERR_PARAM, "Invalid number of channels: %u", num_channels);
        return;
    }

    WaveU16 old_num_channels = self->format_chunk.body.num_channels;
    if (num_channels == old_num_channels)
        return;

    self->format_chunk.body.num_channels = num_channels;
    self->format_chunk.body.block_align = self->format_chunk.body.block_align / old_num_channels * num_channels;
    self->format_chunk.body.avg_bytes_per_sec = self->format_chunk.body.block_align * self->format_chunk.body.sample_rate;

    wave_write_header(self);
}

void wave_set_sample_rate(WaveFile* self, WaveU32 sample_rate)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (sample_rate == self->format_chunk.body.sample_rate)
        return;

    self->format_chunk.body.sample_rate = sample_rate;
    self->format_chunk.body.avg_bytes_per_sec = self->format_chunk.body.block_align * self->format_chunk.body.sample_rate;

    wave_write_header(self);
}

void wave_set_valid_bits_per_sample(WaveFile* self, WaveU16 bits)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (bits < 1 || bits > 8 * self->format_chunk.body.block_align / self->format_chunk.body.num_channels) {
        wave_err_set(WAVE_ERR_PARAM, "Invalid ValidBitsPerSample: %u", bits);
        return;
    }

    if ((self->format_chunk.body.format_tag == WAVE_FORMAT_ALAW || self->format_chunk.body.format_tag == WAVE_FORMAT_MULAW) && bits != 8) {
        wave_err_set(WAVE_ERR_PARAM, "Invalid ValidBitsPerSample: %u", bits);
        return;
    }

    if (self->format_chunk.body.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.bits_per_sample = bits;
    } else {
        self->format_chunk.body.bits_per_sample = 8 * self->format_chunk.body.block_align / self->format_chunk.body.num_channels;
        self->format_chunk.body.valid_bits_per_sample = bits;
    }

    wave_write_header(self);
}

void wave_set_sample_size(WaveFile* self, size_t sample_size)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (sample_size < 1) {
        wave_err_set(WAVE_ERR_PARAM, "Invalid sample size: %zu", sample_size);
        return;
    }

    self->format_chunk.body.block_align = (WaveU16)(sample_size * self->format_chunk.body.num_channels);
    self->format_chunk.body.avg_bytes_per_sec = self->format_chunk.body.block_align * self->format_chunk.body.sample_rate;
    self->format_chunk.body.bits_per_sample = (WaveU16)(sample_size * 8);
    if (self->format_chunk.body.format_tag == WAVE_FORMAT_EXTENSIBLE) {
        self->format_chunk.body.valid_bits_per_sample = (WaveU16)(sample_size * 8);
    }

    wave_write_header(self);
}

void wave_set_channel_mask(WaveFile* self, WaveU32 channel_mask)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (self->format_chunk.body.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Extensible format is not supported");
        return;
    }

    self->format_chunk.body.channel_mask = channel_mask;

    wave_write_header(self);
}

void wave_set_sub_format(WaveFile* self, WaveU16 sub_format)
{
    if (!(self->mode & WAVE_OPEN_WRITE) && !((self->mode & WAVE_OPEN_APPEND) && self->is_a_new_file && self->data_chunk.header.size == 0)) {
        wave_err_set_literal(WAVE_ERR_MODE, "This WaveFile is not writable");
        return;
    }

    if (self->format_chunk.body.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        wave_err_set_literal(WAVE_ERR_FORMAT, "Extensible format is not supported");
        return;
    }

    self->format_chunk.body.sub_format[0] = (WaveU8)(sub_format & 0xff);
    self->format_chunk.body.sub_format[1] = (WaveU8)(sub_format >> 8);

    wave_write_header(self);
}

WaveU16 wave_get_format(WAVE_CONST WaveFile* self)
{
    return self->format_chunk.body.format_tag;
}

WaveU16 wave_get_num_channels(WAVE_CONST WaveFile* self)
{
    return self->format_chunk.body.num_channels;
}

WaveU32 wave_get_sample_rate(WAVE_CONST WaveFile* self)
{
    return self->format_chunk.body.sample_rate;
}

WaveU16 wave_get_valid_bits_per_sample(WAVE_CONST WaveFile* self)
{
    if (self->format_chunk.body.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        return self->format_chunk.body.bits_per_sample;
    } else {
        return self->format_chunk.body.valid_bits_per_sample;
    }
}

size_t wave_get_sample_size(WAVE_CONST WaveFile* self)
{
    return self->format_chunk.body.block_align / self->format_chunk.body.num_channels;
}

size_t wave_get_length(WAVE_CONST WaveFile* self)
{
    return self->data_chunk.header.size / (self->format_chunk.body.block_align);
}

WaveU32 wave_get_channel_mask(WAVE_CONST WaveFile* self)
{
    return self->format_chunk.body.channel_mask;
}

WaveU16 wave_get_sub_format(WAVE_CONST WaveFile* self)
{
    WaveU16 sub_format = self->format_chunk.body.sub_format[1];
    sub_format <<= 8;
    sub_format |= self->format_chunk.body.sub_format[0];
    return sub_format;
}
