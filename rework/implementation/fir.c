#include <stdint.h>
#include <limits.h>
#include <math.h>
#include "fir.h"

/* Multiply two 16-bit values and accumulate into 32-bit sum */
static inline int32_t signed_multipy_accumulate(int32_t sum, int16_t op_1, int16_t op_2)
{
    return sum + (((int32_t)op_1 * (int32_t)op_2) << 1);
}

/* Basic FIR filter implementation */
int16_t fir_basic(int16_t input, int16_t* coeffs, int16_t *history, uint16_t n_coeff)
{
    int32_t acc = 0;

    /* Shift history array to the right */
    for (int i = n_coeff - 1; i > 0; i--)
        history[i] = history[i - 1];

    /* Insert new sample at the beginning */
    history[0] = input;

    /* Multiply-accumulate operation */
    for (int i = 0; i < n_coeff; i++)
        acc = signed_multipy_accumulate(acc, history[i], coeffs[i]);

    /* Scaling */
    acc >>= 16;

    /* Saturation */
    if (acc > INT16_MAX) acc = INT16_MAX;
    if (acc < INT16_MIN) acc = INT16_MIN;

    return (int16_t)acc;
}
