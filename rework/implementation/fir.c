#include <stdint.h>
#include <limits.h>
#include <math.h>
#include "fir.h"

static inline int32_t signed_multipy_accumulate(int32_t sum, int16_t op_1, int16_t op_2)
{
    return sum + (((int32_t)op_1 * (int32_t)op_2) << 1);
}


int16_t fir_basic(int16_t input, int16_t* coeffs, int16_t *history, uint16_t n_coeff)
{
    int32_t acc = 0;

    /* pomeranje history udesno */
    for (int i = n_coeff - 1; i > 0; i--)
        history[i] = history[i - 1];

    /* novi uzorak na početak */
    history[0] = input;

    /* MAC operacije */
    for (int i = 0; i < n_coeff; i++)
        acc = signed_multipy_accumulate(acc, history[i], coeffs[i]);

    /* Q31 - Q15 skaliranje */
    acc >>= 16;

    /* saturacija */
    if (acc > INT16_MAX) acc = INT16_MAX;
    if (acc < INT16_MIN) acc = INT16_MIN;

    return (int16_t)acc;
}
