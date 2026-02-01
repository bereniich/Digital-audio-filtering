#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <limits.h>

#include "wav.h"
#include "fir.h"
#include "iir.h"
#include "low_pass_coefficients.h"

#define AUDIO_IO_SIZE 256
#define INPUT_FILE "../../streams/7.wav"

/* -------------------------------------------------------------------------- */
/* Utility */
static void ensure_output_dir(void) {
    struct stat st = {0};
    if (stat("output", &st) == -1) {
        if (mkdir("output", 0755) != 0) {
            perror("mkdir");
            exit(EXIT_FAILURE);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* FIR history buffers */
static int16_t h35L[LP35_LENGTH]={0}, h35R[LP35_LENGTH]={0};
static int16_t h77L[LP77_LENGTH]={0}, h77R[LP77_LENGTH]={0};
static int16_t h129L[LP129_LENGTH]={0}, h129R[LP129_LENGTH]={0};

/* -------------------------------------------------------------------------- */
/* IIR STATES — SVAKA KOMBINACIJA IMA SVOJE STANJE */

/* IIR 2nd order */
int16_t x2_35_L[2]={0}, y2_35_L[2]={0}, x2_35_R[2]={0}, y2_35_R[2]={0};
int16_t x2_77_L[2]={0}, y2_77_L[2]={0}, x2_77_R[2]={0}, y2_77_R[2]={0};
int16_t x2_129_L[2]={0}, y2_129_L[2]={0}, x2_129_R[2]={0}, y2_129_R[2]={0};

/* IIR 4th order */
int16_t x4_35_L[2][2]={0}, y4_35_L[2][2]={0}, x4_35_R[2][2]={0}, y4_35_R[2][2]={0};
int16_t x4_77_L[2][2]={0}, y4_77_L[2][2]={0}, x4_77_R[2][2]={0}, y4_77_R[2][2]={0};
int16_t x4_129_L[2][2]={0}, y4_129_L[2][2]={0}, x4_129_R[2][2]={0}, y4_129_R[2][2]={0};

/* IIR 6th order */
int16_t x6_35_L[3][2]={0}, y6_35_L[3][2]={0}, x6_35_R[3][2]={0}, y6_35_R[3][2]={0};
int16_t x6_77_L[3][2]={0}, y6_77_L[3][2]={0}, x6_77_R[3][2]={0}, y6_77_R[3][2]={0};
int16_t x6_129_L[3][2]={0}, y6_129_L[3][2]={0}, x6_129_R[3][2]={0}, y6_129_R[3][2]={0};

/* -------------------------------------------------------------------------- */
/* IIR COEFFICIENTS (isti biquad korišćen u kaskadi) */

int16_t coeff2[6] = {
    INT16_MAX,
    (int16_t)(-1.998/2 * INT16_MAX),
    INT16_MAX,
    INT16_MAX,
    (int16_t)(-1.937/2 * INT16_MAX),
    (int16_t)(0.9409 * INT16_MAX)
};

int16_t coeff4[2][6] = { 
    { INT16_MAX,(int16_t)(-1.998/2*INT16_MAX),INT16_MAX,INT16_MAX,(int16_t)(-1.937/2*INT16_MAX),(int16_t)(0.9409*INT16_MAX) },
    { INT16_MAX,(int16_t)(-1.998/2*INT16_MAX),INT16_MAX,INT16_MAX,(int16_t)(-1.937/2*INT16_MAX),(int16_t)(0.9409*INT16_MAX) }
};

int16_t coeff6[3][6] = {
    { INT16_MAX,(int16_t)(-1.998/2*INT16_MAX),INT16_MAX,INT16_MAX,(int16_t)(-1.937/2*INT16_MAX),(int16_t)(0.9409*INT16_MAX) },
    { INT16_MAX,(int16_t)(-1.998/2*INT16_MAX),INT16_MAX,INT16_MAX,(int16_t)(-1.937/2*INT16_MAX),(int16_t)(0.9409*INT16_MAX) },
    { INT16_MAX,(int16_t)(-1.998/2*INT16_MAX),INT16_MAX,INT16_MAX,(int16_t)(-1.937/2*INT16_MAX),(int16_t)(0.9409*INT16_MAX) }
};

/* -------------------------------------------------------------------------- */
int main(void)
{
    ensure_output_dir();

    FILE *fin = fopen(INPUT_FILE,"rb");
    if(!fin){ perror("input"); return 1; }

    WAVHeader hdr;
    read_wav_header(fin,&hdr);

    FILE *f_fir35      = fopen("output/fir35.wav","wb+");
    FILE *f_fir77      = fopen("output/fir77.wav","wb+");
    FILE *f_fir129     = fopen("output/fir129.wav","wb+");

    FILE *f_35_iir2    = fopen("output/fir35_iir2.wav","wb+");
    FILE *f_35_iir4    = fopen("output/fir35_iir4.wav","wb+");
    FILE *f_35_iir6    = fopen("output/fir35_iir6.wav","wb+");

    FILE *f_77_iir2    = fopen("output/fir77_iir2.wav","wb+");
    FILE *f_77_iir4    = fopen("output/fir77_iir4.wav","wb+");
    FILE *f_77_iir6    = fopen("output/fir77_iir6.wav","wb+");

    FILE *f_129_iir2   = fopen("output/fir129_iir2.wav","wb+");
    FILE *f_129_iir4   = fopen("output/fir129_iir4.wav","wb+");
    FILE *f_129_iir6   = fopen("output/fir129_iir6.wav","wb+");

    FILE *outs[] = {
        f_fir35,
        f_fir77,
        f_fir129,
        f_35_iir2,
        f_35_iir4,
        f_35_iir6,
        f_77_iir2,
        f_77_iir4,
        f_77_iir6,
        f_129_iir2,
        f_129_iir4,
        f_129_iir6
    };

    for(int i=0;i<12;i++) 
        fwrite(&hdr,sizeof(WAVHeader),1,outs[i]);

    int16_t in[AUDIO_IO_SIZE*2], L[AUDIO_IO_SIZE], R[AUDIO_IO_SIZE];
    int16_t o[12][AUDIO_IO_SIZE*2];
    size_t frames,total[12]={0};

    while((frames=fread(in,hdr.blockAlign,AUDIO_IO_SIZE,fin))>0)
    {
        for(size_t i=0;i<frames;i++){ L[i]=in[2*i]; R[i]=in[2*i+1]; }

        for(size_t i=0;i<frames;i++)
        {
            int16_t f35L = fir_basic(L[i],lowpass_35_coeffs,h35L,LP35_LENGTH);
            int16_t f35R = fir_basic(R[i],lowpass_35_coeffs,h35R,LP35_LENGTH);

            int16_t f77L = fir_basic(L[i],lowpass_77_coeffs,h77L,LP77_LENGTH);
            int16_t f77R = fir_basic(R[i],lowpass_77_coeffs,h77R,LP77_LENGTH);

            int16_t f129L = fir_basic(L[i],lowpass_129_coeffs,h129L,LP129_LENGTH);
            int16_t f129R = fir_basic(R[i],lowpass_129_coeffs,h129R,LP129_LENGTH);

            int16_t outL[12]={
                f35L,f77L,f129L,
                second_order_IIR(f35L,coeff2,x2_35_L,y2_35_L),
                fourth_order_IIR(f35L,coeff4,x4_35_L,y4_35_L),
                sixth_order_IIR(f35L,coeff6,x6_35_L,y6_35_L),
                second_order_IIR(f77L,coeff2,x2_77_L,y2_77_L),
                fourth_order_IIR(f77L,coeff4,x4_77_L,y4_77_L),
                sixth_order_IIR(f77L,coeff6,x6_77_L,y6_77_L),
                second_order_IIR(f129L,coeff2,x2_129_L,y2_129_L),
                fourth_order_IIR(f129L,coeff4,x4_129_L,y4_129_L),
                sixth_order_IIR(f129L,coeff6,x6_129_L,y6_129_L)
            };

            int16_t outR[12]={
                f35R,f77R,f129R,
                second_order_IIR(f35R,coeff2,x2_35_R,y2_35_R),
                fourth_order_IIR(f35R,coeff4,x4_35_R,y4_35_R),
                sixth_order_IIR(f35R,coeff6,x6_35_R,y6_35_R),
                second_order_IIR(f77R,coeff2,x2_77_R,y2_77_R),
                fourth_order_IIR(f77R,coeff4,x4_77_R,y4_77_R),
                sixth_order_IIR(f77R,coeff6,x6_77_R,y6_77_R),
                second_order_IIR(f129R,coeff2,x2_129_R,y2_129_R),
                fourth_order_IIR(f129R,coeff4,x4_129_R,y4_129_R),
                sixth_order_IIR(f129R,coeff6,x6_129_R,y6_129_R)
            };

            for(int k=0;k<12;k++){
                o[k][2*i]=outL[k];
                o[k][2*i+1]=outR[k];
            }
        }

        for(int k=0;k<12;k++){
            fwrite(o[k],hdr.blockAlign,frames,outs[k]);
            total[k]+=frames;
        }
    }

    fclose(fin);
    return 0;
}
