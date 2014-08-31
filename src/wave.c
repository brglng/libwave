#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wave.h"
#include "wave_priv.h"

static const char default_sub_format[16] = {
    0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

size_t wave_get_header_size(WaveFile* wave)
{
    size_t header_size = WAVE_RIFF_HEADER_SIZE + 4 +
                         WAVE_RIFF_HEADER_SIZE + wave->chunk.format_chunk.size +
                         WAVE_RIFF_HEADER_SIZE;

    if (wave->chunk.fact_chunk.id == WAVE_FACT_CHUNK_ID) {
        header_size += WAVE_RIFF_HEADER_SIZE + wave->chunk.fact_chunk.size;
    }

    return header_size;
}

void wave_parse_header(WaveFile* self)
{
    size_t read_count;

    read_count = fread(&self->chunk, WAVE_RIFF_HEADER_SIZE, 1, self->fp);
    if (read_count != 1 || self->chunk.id != WAVE_RIFF_CHUNK_ID) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    read_count = fread(&self->chunk.wave_id, 4, 1, self->fp);
    if (read_count != 1 || self->chunk.wave_id != WAVE_WAVE_ID) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    read_count = fread(&self->chunk.format_chunk, WAVE_RIFF_HEADER_SIZE, 1, self->fp);
    if (read_count != 1 || self->chunk.format_chunk.id != WAVE_FORMAT_CHUNK_ID) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    read_count = fread((uint8_t*)(&self->chunk.format_chunk) + WAVE_RIFF_HEADER_SIZE,
                       self->chunk.format_chunk.size, 1, self->fp);
    if (read_count != 1 ) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    if (self->chunk.format_chunk.format_tag == WAVE_FORMAT_PCM) {
    } else if (self->chunk.format_chunk.format_tag == WAVE_FORMAT_IEEE_FLOAT) {
    } else if (self->chunk.format_chunk.format_tag == WAVE_FORMAT_ALAW ||
               self->chunk.format_chunk.format_tag == WAVE_FORMAT_MULAW) {
    } else {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    read_count = fread(&self->chunk.fact_chunk, WAVE_RIFF_HEADER_SIZE, 1, self->fp);
    if (read_count != 1) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    if (self->chunk.fact_chunk.id == WAVE_FACT_CHUNK_ID) {
        read_count = fread((uint8_t*)(&self->chunk.fact_chunk) + WAVE_RIFF_HEADER_SIZE,
                           self->chunk.fact_chunk.size, 1, self->fp);
        if (read_count != 1) {
            self->error_code = WAVE_ERROR_FORMAT;
            return;
        }

        read_count = fread(&self->chunk.data_chunk, WAVE_RIFF_HEADER_SIZE, 1, self->fp);
        if (read_count != 1) {
            self->error_code = WAVE_ERROR_FORMAT;
            return;
        }
    } else if (self->chunk.fact_chunk.id == WAVE_DATA_CHUNK_ID) {
        self->chunk.data_chunk.id = self->chunk.fact_chunk.id;
        self->chunk.data_chunk.size = self->chunk.fact_chunk.size;
        self->chunk.fact_chunk.size = 0;
    }

    self->error_code = WAVE_SUCCESS;
}

void wave_write_header(WaveFile* self)
{
    size_t write_count;

    fseek(self->fp, 0, SEEK_SET);

    self->chunk.size = (4) +
                       (WAVE_RIFF_HEADER_SIZE + self->chunk.format_chunk.size) +
                       (WAVE_RIFF_HEADER_SIZE + self->chunk.data_chunk.size);

    if (self->chunk.fact_chunk.id == WAVE_FACT_CHUNK_ID) {
        /* if there is a fact chunk */
        self->chunk.size += WAVE_RIFF_HEADER_SIZE + self->chunk.fact_chunk.size;
    }

    if (self->chunk.size % 2 != 0) {
        self->chunk.size++;
    }

    write_count = fwrite(&self->chunk, WAVE_RIFF_HEADER_SIZE + 4, 1, self->fp);
    if (write_count != 1) {
        self->error_code = WAVE_ERROR_STDIO;
        return;
    }

    write_count = fwrite(&self->chunk.format_chunk, WAVE_RIFF_HEADER_SIZE + self->chunk.format_chunk.size, 1, self->fp);
    if (write_count != 1) {
        self->error_code = WAVE_ERROR_STDIO;
        return;
    }

    if (self->chunk.fact_chunk.id == WAVE_FACT_CHUNK_ID) {
        /* if there is a fact chunk */
        write_count = fwrite(&self->chunk.fact_chunk, WAVE_RIFF_HEADER_SIZE + self->chunk.fact_chunk.size, 1, self->fp);
        if (write_count != 1) {
            self->error_code = WAVE_ERROR_STDIO;
            return;
        }
    }

    write_count = fwrite(&self->chunk.data_chunk, WAVE_RIFF_HEADER_SIZE, 1, self->fp);
    if (write_count != 1) {
        self->error_code = WAVE_ERROR_STDIO;
        return;
    }

    self->error_code = WAVE_SUCCESS;
}

void wave_init(WaveFile* self, char* filename, char* mode)
{
    self->fp = NULL;
    self->error_code = WAVE_SUCCESS;
    memset(&self->chunk, 0, sizeof(WaveMasterChunk));

    if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0) {
        self->mode = "rb";
    } else if (strcmp(mode, "r+") == 0 || strcmp(mode, "rb+") == 0 || strcmp(mode, "r+b") == 0) {
        self->mode = "rb+";
    } else if (strcmp(mode, "w") == 0 || strcmp(mode, "wb") == 0) {
        self->mode = "wb";
    } else if (strcmp(mode, "w+") == 0 || strcmp(mode, "wb+") == 0 || strcmp(mode, "w+b") == 0) {
        self->mode = "wb+";
    } else if (strcmp(mode, "wx") == 0 || strcmp(mode, "wbx") == 0) {
        self->mode = "wbx";
    } else if (strcmp(mode, "w+x") == 0 || strcmp(mode, "wb+x") == 0 || strcmp(mode, "w+bx") == 0) {
        self->mode = "wb+x";
    } else if (strcmp(mode, "a") == 0 || strcmp(mode, "ab") == 0) {
        self->mode = "ab";
    } else if (strcmp(mode, "a+") == 0 || strcmp(mode, "ab+") == 0 || strcmp(mode, "a+b") == 0) {
        self->mode = "ab+";
    } else {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    self->fp = fopen(filename, self->mode);
    if (self->fp == NULL) {
        self->error_code = WAVE_ERROR_STDIO;
        return;
    }

    if (self->mode[0] == 'r') {
        wave_parse_header(self);
        return;
    }

    if (self->mode[0] == 'a') {
        wave_parse_header(self);
        if (self->error_code == WAVE_SUCCESS) {
            // If it succeeded in parsing header, return immediately.
            return;
        } else {
            // Header parsing failed. Regard it as a new file.
            rewind(self->fp);
        }
    }

    // reaches here only if (mode[0] == 'w' || mode[0] == 'a')

    self->chunk.id = WAVE_RIFF_CHUNK_ID;
    /* self->chunk.size = calculated by wave_write_header */
    self->chunk.wave_id = WAVE_WAVE_ID;

    self->chunk.format_chunk.id = WAVE_FORMAT_CHUNK_ID;
    self->chunk.format_chunk.size = (size_t)&self->chunk.format_chunk.ext_size - (size_t)&self->chunk.format_chunk.format_tag;
    self->chunk.format_chunk.format_tag = WAVE_FORMAT_PCM;
    self->chunk.format_chunk.n_channels = 2;
    self->chunk.format_chunk.sample_rate = 44100;
    self->chunk.format_chunk.avg_bytes_per_sec = 44100*2*2;
    self->chunk.format_chunk.block_align = 4;
    self->chunk.format_chunk.bits_per_sample = 16;

    memcpy(self->chunk.format_chunk.sub_format, default_sub_format, 16);

    self->chunk.fact_chunk.id = WAVE_DATA_CHUNK_ID;     /* flag to indicate there is no fact chunk */
    self->chunk.fact_chunk.size = 0;

    self->chunk.data_chunk.id = WAVE_DATA_CHUNK_ID;
    self->chunk.data_chunk.size = 0;

    wave_write_header(self);

    if (self->error_code != WAVE_SUCCESS) {
        return;
    }

    self->error_code = WAVE_SUCCESS;
}

void wave_finalize(WaveFile* self)
{
    int ret;

    if (self->fp == NULL) {
        return;
    }

    if ( (strcmp(self->mode, "wb") == 0 ||
          strcmp(self->mode, "wb+") == 0 ||
          strcmp(self->mode, "wbx") == 0 ||
          strcmp(self->mode, "wb+x") == 0 ||
          strcmp(self->mode, "ab") == 0 ||
          strcmp(self->mode, "ab+") == 0) &&
          self->chunk.data_chunk.size % 2 != 0 &&
          wave_eof(self) ) {

        char padding = 0;
        ret = fwrite(&padding, sizeof(padding), 1, self->fp);
        if (ret != 1) {
            self->error_code = WAVE_ERROR_STDIO;
            return;
        }
    }

    ret = fclose(self->fp);
    if (ret != 0) {
        self->error_code = WAVE_ERROR_STDIO;
        return;
    }

    self->error_code = WAVE_SUCCESS;
}

WaveFile* wave_open(char* filename, char* mode)
{
    WaveFile* wave = malloc(sizeof(WaveFile));
    if (wave == NULL) {
        return NULL;
    }

    wave_init(wave, filename, mode);

    //if (wave->error_code != WAVE_SUCCESS) {
    //    return NULL;
    //}

    //wave->error_code = WAVE_SUCCESS;

    return wave;
}

int wave_close(WaveFile* wave)
{
    wave_finalize(wave);

    if (wave->error_code != WAVE_SUCCESS) {
        return EOF;
    }

    free(wave);

    return 0;
}

WaveFile* wave_reopen(char* filename, char* mode, WaveFile* wave)
{
    wave_finalize(wave);
    wave_init(wave, filename, mode);
    return wave;
}

const static wave_container_sizes[5] = {0, 1, 2, 4, 4};

static __inline size_t wave_calc_container_size(size_t sample_size)
{
    return wave_container_sizes[sample_size];
}

size_t wave_read(void** buffers, size_t count, WaveFile* wave)
{
    size_t read_count;
    uint8_t* buffer;
    uint16_t n_channels = wave_get_n_channels(wave);
    size_t sample_size = wave_get_sample_size(wave);
    size_t container_size = wave_calc_container_size(sample_size);
    size_t i, j, k;
    size_t len_remain;

    if (strcmp(wave->mode, "wb") == 0 || strcmp(wave->mode, "wbx") == 0 || strcmp(wave->mode, "ab") == 0) {
        wave->error_code = WAVE_ERROR_MODE;
        return 0;
    }

    if (wave->chunk.format_chunk.format_tag == WAVE_FORMAT_EXTENSIBLE) {
        wave->error_code = WAVE_ERROR_FORMAT;
        return 0;
    }

    len_remain = wave_get_length(wave) - (size_t)wave_tell(wave);
    if (wave->error_code != WAVE_SUCCESS) {
        return 0;
    }
    count = (count <= len_remain) ? count : len_remain;

    if (count == 0) {
        wave->error_code = WAVE_SUCCESS;
        return 0;
    }

    buffer = malloc(n_channels * count * sample_size);
    if (buffer == NULL) {
        wave->error_code = WAVE_ERROR_MALLOC;
        return 0;
    }

    read_count = fread(buffer, sample_size, n_channels * count, wave->fp);
    if (ferror(wave->fp)) {
        free(buffer);
        wave->error_code = WAVE_ERROR_STDIO;
        return 0;
    }

    for (i = 0; i < n_channels; ++i) {
        for (j = 0; j < read_count / n_channels; ++j) {
#ifdef WAVE_ENDIAN_LITTLE
            for (k = 0; k < sample_size; ++k) {
                ((uint8_t*)buffers[i])[j*container_size + k] = buffer[j*n_channels*sample_size + i*sample_size + k];
            }

            /* sign extension */
            if (((uint8_t*)buffers[i])[j*container_size + k-1] & 0x80) {
                for (; k < container_size; ++k) {
                    ((uint8_t*)buffers[i])[j*container_size + k] = 0xff;
                }
            } else {
                for (; k < container_size; ++k) {
                    ((uint8_t*)buffers[i])[j*container_size + k] = 0x00;
                }
            }
#endif
        }
    }

    free(buffer);

    wave->error_code = WAVE_SUCCESS;

    return read_count / n_channels;
}

size_t wave_write(void** buffers, size_t count, WaveFile* wave)
{
    size_t write_count;
    uint8_t* buffer;
    uint16_t n_channels = wave_get_n_channels(wave);
    size_t sample_size = wave_get_sample_size(wave);
    size_t container_size = wave_calc_container_size(sample_size);
    size_t i, j, k;
    long int save_pos;

    if (strcmp(wave->mode, "rb") == 0) {
        wave->error_code = WAVE_ERROR_MODE;
        return 0;
    }

    if (wave->chunk.format_chunk.format_tag == WAVE_FORMAT_EXTENSIBLE) {
        wave->error_code = WAVE_ERROR_FORMAT;
        return 0;
    }

    if (count == 0) {
        wave->error_code = WAVE_SUCCESS;
        return 0;
    }

    wave_tell(wave);
    if (wave->error_code != WAVE_SUCCESS) {
        return 0;
    }

    buffer = malloc(n_channels * count * sample_size);
    if (buffer == NULL) {
        wave->error_code = WAVE_ERROR_MALLOC;
        return 0;
    }

    for (i = 0; i < n_channels; ++i) {
        for (j = 0; j < count; ++j) {
#ifdef WAVE_ENDIAN_LITTLE
            for (k = 0; k < sample_size; ++k) {
                buffer[j*n_channels*sample_size + i*sample_size + k] = ((uint8_t*)buffers[i])[j*container_size + k];
            }
#endif
        }
    }

    write_count = fwrite(buffer, sample_size, n_channels * count, wave->fp);
    if (ferror(wave->fp)) {
        free(buffer);
        wave->error_code = WAVE_ERROR_STDIO;
        return 0;
    }

    free(buffer);

    wave->chunk.data_chunk.size += write_count * sample_size;

    if (wave->chunk.fact_chunk.id == WAVE_FACT_CHUNK_ID) {
        wave->chunk.fact_chunk.sample_length += write_count / n_channels;
    }

    save_pos = ftell(wave->fp);
    if (save_pos == -1L) {
        wave->error_code = WAVE_ERROR_STDIO;
        return 0;
    }
    wave_write_header(wave);
    if (wave->error_code != WAVE_SUCCESS) {
        return 0;
    }
    if (fseek(wave->fp, save_pos, SEEK_SET) != 0) {
        wave->error_code = WAVE_ERROR_STDIO;
        return 0;
    }

    wave->error_code = WAVE_SUCCESS;
    return write_count / n_channels;
}

long int wave_tell(WaveFile* wave)
{
    long pos = ftell(wave->fp);

    if (pos == -1L) {
        wave->error_code = WAVE_ERROR_STDIO;
        return -1L;
    } else {
        size_t header_size = wave_get_header_size(wave);
        if (pos < header_size) {
            wave->error_code = WAVE_ERROR_POS;
            return -1L;
        }

        wave->error_code = WAVE_SUCCESS;
        return (pos - header_size) / (wave->chunk.format_chunk.block_align);
    }
}

int wave_seek(WaveFile* wave, long int offset, int origin)
{
    size_t length = wave_get_length(wave);
    int ret;

    if (origin == SEEK_CUR) {
        offset += wave_tell(wave);
    } else if (origin == SEEK_END) {
        offset += length;
    }

    if (offset >= 0 && offset <= length) {
        offset = wave->chunk.format_chunk.block_align;
    } else {
        wave->error_code = WAVE_ERROR_PARAM;
        return wave->error_code;
    }

    ret = fseek(wave->fp, offset, SEEK_SET);

    if (ret != 0) {
        wave->error_code = WAVE_ERROR_STDIO;
        return wave->error_code;
    }

    wave->error_code = WAVE_SUCCESS;
    return wave->error_code;
}

void wave_rewind(WaveFile* wave)
{
    wave_seek(wave, 0, SEEK_SET);
}

int wave_eof(WaveFile* wave)
{
    return feof(wave->fp) ||
           ftell(wave->fp) == wave_get_header_size(wave) + wave->chunk.data_chunk.size;
}

int wave_error(WaveFile* wave)
{
    return wave->error_code != WAVE_SUCCESS || (wave->fp != NULL && ferror(wave->fp));
}

int wave_flush(WaveFile* wave)
{
    int ret = fflush(wave->fp);

    wave->error_code = (ret == 0) ? WAVE_SUCCESS : WAVE_ERROR_STDIO;

    return ret;
}

void wave_set_format(WaveFile* self, uint16_t format)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    self->chunk.format_chunk.format_tag = format;
    if (format != WAVE_FORMAT_PCM && format != WAVE_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.ext_size = 0;
        self->chunk.format_chunk.size = (size_t)&self->chunk.format_chunk.valid_bits_per_sample - (size_t)&self->chunk.format_chunk.format_tag;
    } else if (format == WAVE_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.ext_size = 22;
        self->chunk.format_chunk.size = sizeof(WaveFormatChunk) - WAVE_RIFF_HEADER_SIZE;
    }

    if (format == WAVE_FORMAT_ALAW || format == WAVE_FORMAT_MULAW) {
        self->chunk.format_chunk.bits_per_sample = 8;
    } else if (format == WAVE_FORMAT_IEEE_FLOAT) {
        if (self->chunk.format_chunk.block_align != 4 && self->chunk.format_chunk.block_align != 8) {
            self->chunk.format_chunk.block_align = 4;
        }
        if (self->chunk.format_chunk.bits_per_sample > 8*self->chunk.format_chunk.block_align) {
            self->chunk.format_chunk.bits_per_sample = 8*self->chunk.format_chunk.block_align;
        }
    }

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

void wave_set_n_channels(WaveFile* self, uint16_t n_channels)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    if (n_channels < 1) {
        self->error_code = WAVE_ERROR_PARAM;
        return;
    }

    self->chunk.format_chunk.n_channels = n_channels;

    self->chunk.format_chunk.avg_bytes_per_sec =
        self->chunk.format_chunk.block_align *
        self->chunk.format_chunk.sample_rate;

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

void wave_set_sample_rate(WaveFile* self, uint32_t sample_rate)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    self->chunk.format_chunk.sample_rate = sample_rate;

    self->chunk.format_chunk.avg_bytes_per_sec =
        self->chunk.format_chunk.block_align *
        self->chunk.format_chunk.sample_rate;

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

void wave_set_valid_bits_per_sample(WaveFile* self, uint16_t bits)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    if (bits < 1 || bits > 8 * self->chunk.format_chunk.block_align / self->chunk.format_chunk.n_channels) {
        self->error_code = WAVE_ERROR_PARAM;
        return;
    }

    if ((self->chunk.format_chunk.format_tag == WAVE_FORMAT_ALAW ||
         self->chunk.format_chunk.format_tag == WAVE_FORMAT_MULAW) &&
        bits != 8) {
        self->error_code = WAVE_ERROR_PARAM;
        return;
    }

    if (self->chunk.format_chunk.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.bits_per_sample = bits;
    } else {
        self->chunk.format_chunk.bits_per_sample = 8 * self->chunk.format_chunk.block_align / self->chunk.format_chunk.n_channels;
        self->chunk.format_chunk.valid_bits_per_sample = bits;
    }

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

void wave_set_sample_size(WaveFile* self, size_t sample_size)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    if (sample_size < 1) {
        self->error_code = WAVE_ERROR_PARAM;
        return;
    }

    self->chunk.format_chunk.block_align = (uint16_t)(sample_size * self->chunk.format_chunk.n_channels);
    self->chunk.format_chunk.avg_bytes_per_sec = self->chunk.format_chunk.block_align * self->chunk.format_chunk.sample_rate;
    self->chunk.format_chunk.bits_per_sample = (uint16_t)(sample_size * 8);
    if (self->chunk.format_chunk.format_tag == WAVE_FORMAT_EXTENSIBLE) {
        self->chunk.format_chunk.valid_bits_per_sample = (uint16_t)(sample_size * 8);
    }

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

void wave_set_channel_mask(WaveFile* self, uint32_t channel_mask)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    if (self->chunk.format_chunk.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    self->chunk.format_chunk.channel_mask = channel_mask;

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

void wave_set_sub_format(WaveFile* self, uint16_t sub_format)
{
    if (self->mode[0] == 'r') {
        self->error_code = WAVE_ERROR_MODE;
        return;
    }

    if (self->chunk.format_chunk.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        self->error_code = WAVE_ERROR_FORMAT;
        return;
    }

    *((uint16_t*)&self->chunk.format_chunk.sub_format) = sub_format;

    wave_write_header(self);
    /* self->error_code is set by wave_write_header */
}

uint16_t wave_get_format(WaveFile* self)
{
    return self->chunk.format_chunk.format_tag;
}

uint16_t wave_get_n_channels(WaveFile* self)
{
    return self->chunk.format_chunk.n_channels;
}

uint32_t wave_get_sample_rate(WaveFile* self)
{
    return self->chunk.format_chunk.sample_rate;
}

uint16_t wave_get_valid_bits_per_sample(WaveFile* self)
{
    if (self->chunk.format_chunk.format_tag != WAVE_FORMAT_EXTENSIBLE) {
        return self->chunk.format_chunk.bits_per_sample;
    } else {
        return self->chunk.format_chunk.valid_bits_per_sample;
    }
}

size_t wave_get_sample_size(WaveFile* self)
{
    return self->chunk.format_chunk.block_align / self->chunk.format_chunk.n_channels;
}

size_t wave_get_length(WaveFile* self)
{
    return self->chunk.data_chunk.size / (self->chunk.format_chunk.block_align);
}

uint32_t wave_get_channel_mask(WaveFile* self)
{
    return self->chunk.format_chunk.channel_mask;
}

uint16_t wave_get_sub_format(WaveFile* self)
{
    return *((uint16_t*)&self->chunk.format_chunk.sub_format);
}

WaveError wave_get_last_error(WaveFile* self)
{
    return self->error_code;
}
