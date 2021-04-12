#include <math.h>
#include <stdlib.h>
#include "wav.h"

void generate_sine_wave(float *x, int sample_rate, int len)
{
    for (int i = 0; i < len; ++i) {
        x[i] = 0.5f * cosf(2 * 3.14159265358979323f * 440.0f * i / sample_rate);
    }
}

int main(void)
{
    float *buf = malloc(sizeof(float) * 10 * 44100);
    generate_sine_wave(buf, 44100, 10 * 44100);
    WavFile *fp = wav_open("out.wav", WAV_OPEN_WRITE);
    wav_set_format(fp, WAV_FORMAT_IEEE_FLOAT);
    /* wav_set_sample_size(fp, sizeof(float)); */
    wav_set_num_channels(fp, 1);
    wav_set_sample_rate(fp, 44100);
    wav_write(fp, buf, 10 * 44100);
    wav_close(fp);
    free(buf);
    return 0;
}
