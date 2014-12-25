#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#endif

#include "wave.h"

#define BLOCKSIZE   256

const static char* appname = "diff-wave";

typedef enum {false, true} bool;

bool blk_conv_to_double(double** to, void** from, uint16_t from_format, size_t from_sample_size, int nch, int blksz)
{
    int i, j;

    if (from_format == WAVE_FORMAT_PCM) {
        if (from_sample_size == 1) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = (double)(((int8_t**)from)[i][j]) / (1<<7);
                }
            }
            return true;
        } else if (from_sample_size == 2) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = (double)(((int16_t**)from)[i][j]) / (1<<15);
                }
            }
            return true;
        } else if (from_sample_size == 3) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = (double)(((int32_t**)from)[i][j]) / (1 << 23);
                }
            }
            return true;
        } else if (from_sample_size == 4) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = (double)(((int32_t**)from)[i][j]) / (1LL << 31);
                }
            }
            return true;
        } else if (from_sample_size == 8) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = (double)(((int64_t**)from)[i][j]) / (1ULL << 63);
                }
            }
            return true;
        }
    } else if (from_format == WAVE_FORMAT_IEEE_FLOAT) {
        if (from_sample_size == 2) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = (double)(((float**)from)[i][j]);
                }
            }
            return true;
        } else if (from_sample_size == 4) {
            for (i=0; i<nch; ++i) {
                for (j=0; j<blksz; ++j) {
                    to[i][j] = ((double**)from)[i][j];
                }
            }
            return true;
        }
    }

    return false;
}

void blk_peak(double* buf, int blksz, double* peak, int* ipeak, size_t sample_cnt)
{
    int i;
    double sample_abs;

    for (i=0; i<blksz; ++i) {
        sample_abs = (buf[i] > 0) ? buf[i] : -buf[i];
        if (sample_abs > *peak) {
            *peak = sample_abs;
            *ipeak = sample_cnt + i;
        }
    }
}

void blk_average(double* buf, int blksz, double* avg, size_t sample_cnt)
{
    int i;
    double sample_abs;

    for (i=0; i<blksz; ++i) {
        sample_abs = (buf[i] > 0) ? buf[i] : -buf[i];
        *avg = ((*avg)*(sample_cnt + i) + sample_abs) / (sample_cnt + i + 1);
    }
}

static inline double db(double amp)
{
    return 20*log10(amp);
}

static inline size_t container_size(size_t sample_size)
{
    size_t ret = 1;
    int i = 0;

    do {
        if (ret >= sample_size) {
            return ret;
        }
        ++i;
        ret <<= 1;
    } while (i < 8*sizeof(size_t));

    return (size_t)1 << (i - 1);
}

bool diff_file(char* in1name, char* in2name, char* outname)
{
    WaveFile* in1 = wave_open(in1name, "r");
    WaveFile* in2 = wave_open(in2name, "r");
    WaveFile* out = wave_open(outname, "w");
    size_t readlen1, readlen2, readlen;
    int nch = wave_get_num_channels(in1);
    void** buf1;
    void** buf2;
    double** buf1_double;
    double** buf2_double;
    double** bufout;
    int i, j;
    double* peaks;
    int* ipeaks;
    double* averages;
    size_t sample_cnt = 0;

    printf("input file 0: %s\n", in1name);
    printf("  format: %#06x\n", wave_get_format(in1));
    printf("  sample size: %u\n", wave_get_sample_size(in1));
    printf("  valid bits per sample: %u\n", wave_get_valid_bits_per_sample(in1));
    printf("  sample rate: %u\n", wave_get_sample_rate(in1));
    printf("  length: %u\n", wave_get_length(in1));
    printf("input file 1: %s\n", in2name);
    printf("  format: %#06x\n", wave_get_format(in2));
    printf("  sample size: %u\n", wave_get_sample_size(in2));
    printf("  valid bits per sample: %u\n", wave_get_valid_bits_per_sample(in2));
    printf("  sample rate: %u\n", wave_get_sample_rate(in2));
    printf("  length: %u\n", wave_get_length(in2));

    if (!in1 || !in2 || !out) {
        printf("  diff failed!\n");
        return false;
    }

    if (nch != wave_get_num_channels(in2) || wave_get_sample_rate(in1) != wave_get_sample_rate(in2)) {
        printf("  diff failed!\n");
        return false;
    }

    buf1 = malloc(nch * sizeof(void*));
    buf2 = malloc(nch * sizeof(void*));
    buf1_double = malloc(nch * sizeof(double*));
    buf2_double = malloc(nch * sizeof(double*));
    bufout = malloc(nch * sizeof(double*));
    peaks = malloc(nch * sizeof(double));
    ipeaks = malloc(nch * sizeof(int));
    averages = malloc(nch * sizeof(double));

    for (i=0; i<nch; ++i) {
        buf1[i] = malloc(container_size(wave_get_sample_size(in1)) * BLOCKSIZE);
        buf2[i] = malloc(container_size(wave_get_sample_size(in2)) * BLOCKSIZE);
        buf1_double[i] = malloc(sizeof(double) * BLOCKSIZE);
        buf2_double[i] = malloc(sizeof(double) * BLOCKSIZE);
        bufout[i] = malloc(sizeof(double) * BLOCKSIZE);
        peaks[i] = 0;
        ipeaks[i] = 0;
        averages[i] = 0;
    }

    wave_set_format(out, WAVE_FORMAT_IEEE_FLOAT);
    wave_set_sample_size(out, 8);
    wave_set_sample_rate(out, wave_get_sample_rate(in1));
    wave_set_num_channels(out, nch);
    printf("output file: %s\n", outname);

    do {
        readlen1 = wave_read(buf1, BLOCKSIZE, in1);
        readlen2 = wave_read(buf2, BLOCKSIZE, in2);

        //system("pause");

        readlen = (readlen1 < readlen2) ? readlen1 : readlen2;

        blk_conv_to_double(buf1_double, buf1, wave_get_format(in1), wave_get_sample_size(in1), nch, readlen);
        blk_conv_to_double(buf2_double, buf2, wave_get_format(in2), wave_get_sample_size(in2), nch, readlen);

        for (i=0; i<nch; ++i) {
            for (j=0; j<readlen; ++j) {
                bufout[i][j] = buf1_double[i][j] - buf2_double[i][j];
            }
            blk_peak(bufout[i], readlen, &peaks[i], &ipeaks[i], sample_cnt);
            blk_average(bufout[i], readlen, &averages[i], sample_cnt);
        }

        wave_write(bufout, readlen, out);

        sample_cnt += readlen;
    } while (readlen == BLOCKSIZE);

    for (i=0; i<nch; ++i) {
        printf("  channel %d max diff = %f dB at frame %d\n", i, db(peaks[i]), ipeaks[i]);
        printf("  channel %d average diff = %f dB\n", i, db(averages[i]));
    }

    for (i=0; i<nch; ++i) {
        free(buf1[i]);
        free(buf2[i]);
        free(buf1_double[i]);
        free(buf2_double[i]);
        free(bufout[i]);
    }
    free(buf1);
    free(buf2);
    free(buf1_double);
    free(buf2_double);
    free(bufout);
    free(peaks);
    free(ipeaks);
    free(averages);

    wave_close(in1);
    wave_close(in2);
    wave_close(out);

    return true;
}

#ifdef _WIN32
bool diff_dir(char* in1name, char* in2name, char* outname, bool recursive)
{
    struct _finddata_t hfile1, hfile2, hfile1_s, hfile2_s;
    intptr_t hfind1, hfind2, hfind1_s, hfind2_s;
    char* in1name_s = NULL;
    char* in2name_s = NULL;
    char* in1name_ss = NULL;
    char* in2name_ss = NULL;
    char* outname_ss = NULL;
    bool ret = false;

    hfind1 = _findfirst(in1name, &hfile1);
    hfind2 = _findfirst(in2name, &hfile2);

    if (!(hfile1.attrib & _A_SUBDIR) && !(hfile2.attrib & _A_SUBDIR)) {
        ret = diff_file(in1name, in2name, outname);
        printf("\n");
    } else if ((hfile1.attrib & _A_SUBDIR) && (hfile2.attrib & _A_SUBDIR)) {
        _mkdir(outname);

        in1name_s = malloc(strlen(in1name) + 3);
        in2name_s = malloc(strlen(in2name) + 3);
        sprintf(in1name_s, "%s\\*", in1name);
        sprintf(in2name_s, "%s\\*", in2name);
        hfind1_s = _findfirst(in1name_s, &hfile1_s);
        do {
            if (strcmp(hfile1_s.name, ".") != 0 && strcmp(hfile1_s.name, "..") != 0) {
                hfind2_s = _findfirst(in2name_s, &hfile2_s);
                do {
                    if (strcmp(hfile2_s.name, ".") != 0 && strcmp(hfile2_s.name, "..") != 0 && strcmp(hfile1_s.name, hfile2_s.name) == 0) {
                        in1name_ss = malloc(strlen(in1name) + strlen(hfile1_s.name) + 2);
                        in2name_ss = malloc(strlen(in2name) + strlen(hfile2_s.name) + 2);
                        outname_ss = malloc(strlen(outname) + strlen(hfile1_s.name) + 2);
                        sprintf(in1name_ss, "%s\\%s", in1name, hfile1_s.name);
                        sprintf(in2name_ss, "%s\\%s", in2name, hfile2_s.name);
                        sprintf(outname_ss, "%s\\%s", outname, hfile1_s.name);
                        if ((hfile1_s.attrib & _A_SUBDIR) && (hfile2_s.attrib & _A_SUBDIR) && recursive) {
                            ret = diff_dir(in1name_ss, in2name_ss, outname_ss, true);
                        } else if (!(hfile1_s.attrib & _A_SUBDIR) && !(hfile2_s.attrib & _A_SUBDIR) &&
                                   strcmp(&hfile1_s.name[strlen(hfile1_s.name) - 4], ".wav") == 0 &&
                                   strcmp(&hfile2_s.name[strlen(hfile2_s.name) - 4], ".wav") == 0) {
                            ret = diff_file(in1name_ss, in2name_ss, outname_ss);
                            printf("\n");
                        }
                        free(in1name_ss);
                        free(in2name_ss);
                        free(outname_ss);
                    }
                } while (_findnext(hfind2_s, &hfile2_s) == 0);
                _findclose(hfind2_s);
            }
        } while (_findnext(hfind1_s, &hfile1_s) == 0);
        _findclose(hfind1_s);
        free(in1name_s);
        free(in2name_s);
    }
    _findclose(hfind1);
    _findclose(hfind2);
    return ret;
}
#endif

void print_help(void)
{
    printf(
        "Usage: %s [-r] <-o OUTFILE> FILE1 FILE2\n"
        "Subtract two wave files and write the output to another wave file.\n"
        "\n"
        "  -o OUTFILE, --output OUTFILE\tspecifies the name of the output file\n"
        "  -r, --recursive             \trecurse into directories; FILE1 FILE2 and\n"
        "                              \tOUTFILE will beregarded as directory names.\n"
        "  -h, --help                  \tprint this help\n"
        "\n"
        "  FILE1                       \tthe name of the input file\n"
        "  FILE2                       \tthe name of the output file\n"
        "\n"
        "The output file is in 64-bit double precision IEEE floating-point format.\n"
        "The report of the diffing result will be printed to stdout.\n",
        appname
    );
    exit(0);
}

void print_usage(void)
{
    printf("Usage: %s [-r] <-o OUTFILE> FILE1 FILE2\n"
           "       %s -h, --help\n",
           appname, appname);
}

int main(int argc, char* argv[])
{
    int i;
    char* in1name = NULL;
    char* in2name = NULL;
    char* outname = NULL;
    bool recursive = false;

    for (i=1; i<argc; ++i) {
        if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
            if (i+1 < argc) {
                outname = argv[i+1];
                ++i;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--recursive") == 0) {
            recursive = true;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
        } else if (in1name == NULL) {
            in1name = argv[i];
        } else if (in2name == NULL) {
            in2name = argv[i];
        } else {
            print_usage();
            exit(-1);
        }
    }

    if (in1name == NULL || in2name == NULL || outname == NULL) {
        print_usage();
        exit(-1);
    }

    diff_dir(in1name, in2name, outname, recursive);

    return 0;
}
