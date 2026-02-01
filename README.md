# Audio Filtering Project – FIR and IIR Filters

## Project Overview

This project implements **digital audio signal processing** using a combination of:

- **FIR low-pass filters** of different lengths
- **IIR notch filters** of 2nd, 4th, and 6th order
- Processing of **stereo 16-bit PCM WAV** audio files

All filters are implemented using **fixed-point arithmetic (int16)** and processed in **block-based streaming mode**.

---

## Input Signal

- Format: **WAV**
- Encoding: **PCM**
- Resolution: **16-bit**
- Channels: **Stereo**
- Input file: `../../streams/7.wav`

---

## FIR Filters

The following **FIR low-pass filters** are implemented:

| FIR Filter | Length | Description |
|-----------|--------|-------------|
| FIR35 | 35 taps | Short filter, gentle transition band |
| FIR77 | 77 taps | Medium selectivity |
| FIR129 | 129 taps | High selectivity, sharp cutoff |

Each FIR filter is applied independently to the left and right audio channels.

---

## IIR Filters

The project uses **IIR notch filters** realized as cascaded **second-order sections (biquads)**:

| IIR Filter | Order | Structure |
|-----------|------|-----------|
| IIR2 | 2nd order | Single biquad |
| IIR4 | 4th order | Two cascaded biquads |
| IIR6 | 6th order | Three cascaded biquads |

Higher-order IIR filters are constructed by cascading identical second-order sections.

---

## FIR–IIR Pairing Strategy

To maintain balanced frequency selectivity and numerical stability, FIR and IIR filters are paired according to their complexity:

| FIR Filter | IIR Filter | Output File |
|-----------|-----------|-------------|
| FIR35 | IIR2 | `output_FIR35_IIR2.wav` |
| FIR77 | IIR4 | `output_FIR77_IIR4.wav` |
| FIR129 | IIR6 | `output_FIR129_IIR6.wav` |

This approach prevents one filter type from dominating the overall response.

---

## Output Files

The following WAV files are generated in the `output/` directory:

### FIR-only outputs
- `output_FIR35.wav`
- `output_FIR77.wav`
- `output_FIR129.wav`

### FIR + IIR cascaded outputs
- `output_FIR35_IIR2.wav`
- `output_FIR77_IIR4.wav`
- `output_FIR129_IIR6.wav`

All output files preserve the original audio format.

---

## Filter Implementation Details

### FIR Filters
- Structure: Direct-form FIR
- Linear phase
- Fixed-point arithmetic (Q15)
- Separate history buffers per channel

### IIR Filters
- Structure: Direct-form II biquad sections
- Implemented using saturation-safe multiply-accumulate operations
- Cascaded realization for higher-order filters
- Separate state memory per biquad and per channel

---

## Block Processing

Audio samples are processed in blocks of:

```c
#define AUDIO_IO_SIZE 256
