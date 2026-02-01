#ifndef WAV_H_
#define WAV_H_

#include <stdint.h>
#include <stdio.h>

// WAV header structure (PCM, little-endian)
typedef struct {
    char     riff[4];         // "RIFF"
    uint32_t chunkSize;
    char     wave[4];         // "WAVE"
    char     fmt[4];          // "fmt "
    uint32_t subchunk1Size;   // 16 for PCM
    uint16_t audioFormat;     // 1 for PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char     data[4];         // "data"
    uint32_t subchunk2Size;
} WAVHeader;

// Write PCM16 WAV file from floating-point buffer (-1..1)

int write_wav(const char *filename,
              const double *buffer,
              size_t n,
              const WAVHeader *header_template);

// Read input wav header data
int read_wav_header(FILE *f, WAVHeader *hdr);

#endif /* WAV_H_ */
