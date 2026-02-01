#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "wav.h"
#include "fir.h"
#include "iir.h"
#include "low_pass_coefficients.h"

#define AUDIO_IO_SIZE 256
#define INPUT_FILE "../../streams/7.wav"

static void ensure_output_dir(void) {
    struct stat st = {0};
    if (stat("output", &st) == -1) {
        if (mkdir("output", 0755) != 0) { perror("mkdir"); exit(EXIT_FAILURE); }
    }
}

// FIR state
static int16_t historyL_35[LP35_LENGTH] = {0};
static int16_t historyR_35[LP35_LENGTH] = {0};
static int16_t historyL_77[LP77_LENGTH] = {0};
static int16_t historyR_77[LP77_LENGTH] = {0};
static int16_t historyL_129[LP129_LENGTH] = {0};
static int16_t historyR_129[LP129_LENGTH] = {0};

// IIR state (2nd order)
static int16_t xHistL2[2] = {0}, yHistL2[2] = {0};
static int16_t xHistR2[2] = {0}, yHistR2[2] = {0};

// IIR state (4th order)
static int16_t xHistL4[2][2] = {0}, yHistL4[2][2] = {0};
static int16_t xHistR4[2][2] = {0}, yHistR4[2][2] = {0};

// IIR state (6th order)
static int16_t xHistL6[3][2] = {0}, yHistL6[3][2] = {0};
static int16_t xHistR6[3][2] = {0}, yHistR6[3][2] = {0};

// 
// IIR coefficients (2nd order)
int16_t coeff2[6] = { 
    1*INT16_MAX, (int16_t)(-1.998/2*INT16_MAX), 1*INT16_MAX, 1*INT16_MAX, (int16_t)(-1.937/2*INT16_MAX), (int16_t)(0.9409*INT16_MAX)
};

// IIR coefficients (4th order = 2 cascaded 2nd-order sections)
int16_t coeff4[2][6] = {
    { 1*INT16_MAX, (int16_t)(-1.998/2*INT16_MAX), 1*INT16_MAX, 1*INT16_MAX, (int16_t)(-1.937/2*INT16_MAX), (int16_t)(0.9409*INT16_MAX) },
    { 1*INT16_MAX, (int16_t)(-1.998/2*INT16_MAX), 1*INT16_MAX, 1*INT16_MAX, (int16_t)(-1.937/2*INT16_MAX), (int16_t)(0.9409*INT16_MAX) }
};

// IIR coefficients (6th order = 3 cascaded 2nd-order sections)
int16_t coeff6[3][6] = {
    { 1*INT16_MAX, (int16_t)(-1.998/2*INT16_MAX), 1*INT16_MAX, 1*INT16_MAX, (int16_t)(-1.937/2*INT16_MAX), (int16_t)(0.9409*INT16_MAX) },
    { 1*INT16_MAX, (int16_t)(-1.998/2*INT16_MAX), 1*INT16_MAX, 1*INT16_MAX, (int16_t)(-1.937/2*INT16_MAX), (int16_t)(0.9409*INT16_MAX) },
    { 1*INT16_MAX, (int16_t)(-1.998/2*INT16_MAX), 1*INT16_MAX, 1*INT16_MAX, (int16_t)(-1.937/2*INT16_MAX), (int16_t)(0.9409*INT16_MAX) }
};

int main(void)
{
    ensure_output_dir();

    FILE *fin = fopen(INPUT_FILE, "rb");
    if (!fin) { perror("fopen input"); return EXIT_FAILURE; }

    WAVHeader input_hdr;
    if (read_wav_header(fin, &input_hdr) != 0) { fclose(fin); return EXIT_FAILURE; }
    if (input_hdr.audioFormat != 1 || input_hdr.bitsPerSample != 16 || input_hdr.numChannels != 2) {
        fprintf(stderr, "Unsupported WAV format\n"); fclose(fin); return EXIT_FAILURE;
    }

    // Output files
    WAVHeader hdr35 = input_hdr, hdr35_2 = input_hdr, hdr35_4 = input_hdr, hdr35_6 = input_hdr;
    WAVHeader hdr77 = input_hdr, hdr129 = input_hdr;

    FILE *fout35 = fopen("output/output_FIR35.wav","wb+");
    FILE *fout77 = fopen("output/output_FIR77.wav","wb+");
    FILE *fout129 = fopen("output/output_FIR129.wav","wb+");
    FILE *fout35_IIR2 = fopen("output/output_FIR35_IIR2.wav","wb+");
    FILE *fout77_IIR4 = fopen("output/output_FIR77_IIR4.wav","wb+");
    FILE *fout129_IIR6 = fopen("output/output_FIR129_IIR6.wav","wb+");
    
    fwrite(&hdr35,sizeof(WAVHeader),1,fout35);
    fwrite(&hdr35_2,sizeof(WAVHeader),1,fout35_IIR2);
    fwrite(&hdr35_4,sizeof(WAVHeader),1,fout77_IIR4);
    fwrite(&hdr35_6,sizeof(WAVHeader),1,fout129_IIR6);
    fwrite(&hdr77,sizeof(WAVHeader),1,fout77);
    fwrite(&hdr129,sizeof(WAVHeader),1,fout129);

    int16_t interleaved[AUDIO_IO_SIZE*2];
    int16_t left[AUDIO_IO_SIZE], right[AUDIO_IO_SIZE];
    int16_t buf35L[AUDIO_IO_SIZE], buf35R[AUDIO_IO_SIZE];
    int16_t bufIIR2L[AUDIO_IO_SIZE], bufIIR2R[AUDIO_IO_SIZE];
    int16_t bufIIR4L[AUDIO_IO_SIZE], bufIIR4R[AUDIO_IO_SIZE];
    int16_t bufIIR6L[AUDIO_IO_SIZE], bufIIR6R[AUDIO_IO_SIZE];
    int16_t buf77L[AUDIO_IO_SIZE], buf77R[AUDIO_IO_SIZE];
    int16_t buf129L[AUDIO_IO_SIZE], buf129R[AUDIO_IO_SIZE];

    int16_t inter35[AUDIO_IO_SIZE*2], interIIR2[AUDIO_IO_SIZE*2], interIIR4[AUDIO_IO_SIZE*2], interIIR6[AUDIO_IO_SIZE*2];
    int16_t inter77[AUDIO_IO_SIZE*2], inter129[AUDIO_IO_SIZE*2];

    size_t total35=0, totalIIR2=0, totalIIR4=0, totalIIR6=0, total77=0, total129=0;
    size_t frames;

    while ((frames = fread(interleaved, input_hdr.blockAlign, AUDIO_IO_SIZE, fin)) > 0)
    {
        // Deinterleave
        for(size_t i=0;i<frames;i++) { left[i]=interleaved[2*i]; right[i]=interleaved[2*i+1]; }

        // FIR 35
        for(size_t i=0;i<frames;i++){
            buf35L[i] = fir_basic(left[i], lowpass_35_coeffs, historyL_35, LP35_LENGTH);
            buf35R[i] = fir_basic(right[i], lowpass_35_coeffs, historyR_35, LP35_LENGTH);
        }

        // FIR 77
        for(size_t i=0;i<frames;i++){
            buf77L[i] = fir_basic(left[i], lowpass_77_coeffs, historyL_77, LP77_LENGTH);
            buf77R[i] = fir_basic(right[i], lowpass_77_coeffs, historyR_77, LP77_LENGTH);
        }

        // FIR 129
        for(size_t i=0;i<frames;i++){
            buf129L[i] = fir_basic(left[i], lowpass_129_coeffs, historyL_129, LP129_LENGTH);
            buf129R[i] = fir_basic(right[i], lowpass_129_coeffs, historyR_129, LP129_LENGTH);
        }

        // 2nd order IIR
        for(size_t i=0;i<frames;i++){
            bufIIR2L[i] = second_order_IIR(buf35L[i], coeff2, xHistL2, yHistL2);
            bufIIR2R[i] = second_order_IIR(buf35R[i], coeff2, xHistR2, yHistR2);
        }

        // 4th order IIR
        for(size_t i=0;i<frames;i++){
            bufIIR4L[i] = fourth_order_IIR(buf77L[i], coeff4, xHistL4, yHistL4);
            bufIIR4R[i] = fourth_order_IIR(buf77R[i], coeff4, xHistR4, yHistR4);
        }

        // 6th order IIR
        for(size_t i=0;i<frames;i++){
            bufIIR6L[i] = sixth_order_IIR(buf129L[i], coeff6, xHistL6, yHistL6);
            bufIIR6R[i] = sixth_order_IIR(buf129R[i], coeff6, xHistR6, yHistR6);
        }

        // Interleave & write
        for(size_t i=0;i<frames;i++){
            inter35[2*i]=buf35L[i]; inter35[2*i+1]=buf35R[i];
            interIIR2[2*i]=bufIIR2L[i]; interIIR2[2*i+1]=bufIIR2R[i];
            interIIR4[2*i]=bufIIR4L[i]; interIIR4[2*i+1]=bufIIR4R[i];
            interIIR6[2*i]=bufIIR6L[i]; interIIR6[2*i+1]=bufIIR6R[i];
            inter77[2*i]=buf77L[i]; inter77[2*i+1]=buf77R[i];
            inter129[2*i]=buf129L[i]; inter129[2*i+1]=buf129R[i];
        }

        fwrite(inter35, input_hdr.blockAlign, frames, fout35);
        fwrite(interIIR2, input_hdr.blockAlign, frames, fout35_IIR2);
        fwrite(interIIR4, input_hdr.blockAlign, frames, fout77_IIR4);
        fwrite(interIIR6, input_hdr.blockAlign, frames, fout129_IIR6);
        fwrite(inter77, input_hdr.blockAlign, frames, fout77);
        fwrite(inter129, input_hdr.blockAlign, frames, fout129);

        total35+=frames; totalIIR2+=frames; totalIIR4+=frames; totalIIR6+=frames; total77+=frames; total129+=frames;
    }

    // Fix headers
    hdr35.subchunk2Size = total35*input_hdr.blockAlign; hdr35.chunkSize = 36+hdr35.subchunk2Size; 
    fseek(fout35,0,SEEK_SET); 
    fwrite(&hdr35,sizeof(WAVHeader),1,fout35);

    hdr35_2.subchunk2Size = totalIIR2*input_hdr.blockAlign; hdr35_2.chunkSize = 36+hdr35_2.subchunk2Size; 
    fseek(fout35_IIR2,0,SEEK_SET); 
    fwrite(&hdr35_2,sizeof(WAVHeader),1,fout35_IIR2);

    hdr35_4.subchunk2Size = totalIIR4*input_hdr.blockAlign; hdr35_4.chunkSize = 36+hdr35_4.subchunk2Size; 
    fseek(fout77_IIR4,0,SEEK_SET); 
    fwrite(&hdr35_4,sizeof(WAVHeader),1,fout77_IIR4);

    hdr35_6.subchunk2Size = totalIIR6*input_hdr.blockAlign; hdr35_6.chunkSize = 36+hdr35_6.subchunk2Size; 
    fseek(fout129_IIR6,0,SEEK_SET); 
    fwrite(&hdr35_6,sizeof(WAVHeader),1,fout129_IIR6);

    hdr77.subchunk2Size = total77*input_hdr.blockAlign; hdr77.chunkSize = 36+hdr77.subchunk2Size; 
    fseek(fout77,0,SEEK_SET); 
    fwrite(&hdr77,sizeof(WAVHeader),1,fout77);

    hdr129.subchunk2Size = total129*input_hdr.blockAlign; hdr129.chunkSize = 36+hdr129.subchunk2Size; 
    fseek(fout129,0,SEEK_SET); 
    fwrite(&hdr129,sizeof(WAVHeader),1,fout129);

    fclose(fout35); 
    fclose(fout35_IIR2); 
    fclose(fout77_IIR4); 
    fclose(fout129_IIR6); 
    fclose(fout77); 
    fclose(fout129);
    fclose(fin);

    return 0;
}
