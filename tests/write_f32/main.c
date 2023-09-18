#include <math.h>
#include <stdlib.h>
#include "wave.h"

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
    WaveFile *fp = wave_open("out.wav", WAVE_OPEN_WRITE);
    wave_set_format(fp, WAVE_FORMAT_IEEE_FLOAT);
    /* wave_set_sample_size(fp, sizeof(float)); */
    wave_set_num_channels(fp, 1);
    wave_set_sample_rate(fp, 44100);
    wave_write(fp, buf, 10 * 44100);
    wave_close(fp);
    free(buf);
    return 0;
}
