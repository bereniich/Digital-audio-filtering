#include "wav.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

int write_wav(const char *filename,
              const double *buffer,
              size_t n,
              const WAVHeader *header_template)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("fopen");
        return -1;
    }

    WAVHeader header = *header_template; // copy provided header

    // Update size fields based on number of samples
    header.subchunk2Size = n * header.blockAlign;
    header.chunkSize     = 36 + header.subchunk2Size;

    // Write header
    fwrite(&header, sizeof(WAVHeader), 1, f);

    // Convert double [-1..1] to PCM16
    for (size_t i = 0; i < n * header.numChannels; i++) {
        double x = buffer[i];
        if (x > 1.0) x = 1.0;
        if (x < -1.0) x = -1.0;
        int16_t sample = (int16_t)lrint(x * 32767.0);
        fwrite(&sample, sizeof(int16_t), 1, f);
    }

    fclose(f);
    return 0;
}

int read_wav_header(FILE *f, WAVHeader *hdr) {
    if (!f || !hdr) return -1;

    // Rewind in case file was already read
    fseek(f, 0, SEEK_SET);

    if (fread(hdr, sizeof(WAVHeader), 1, f) != 1) {
        return -1;
    }

    // Basic checks
    if (strncmp(hdr->riff, "RIFF", 4) != 0 ||
        strncmp(hdr->wave, "WAVE", 4) != 0 ||
        strncmp(hdr->fmt,  "fmt ", 4) != 0 ||
        strncmp(hdr->data, "data", 4) != 0) {
        fprintf(stderr, "Invalid WAV file header\n");
        return -1;
    }

    if (hdr->audioFormat != 1) {
        fprintf(stderr, "Unsupported WAV format (only PCM=1 allowed)\n");
        return -1;
    }

    return 0;
}
