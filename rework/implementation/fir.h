#ifndef IMPLEMENTATION_FIR_H_
#define IMPLEMENTATION_FIR_H_

#include <stdint.h>

int16_t fir_basic(int16_t input, int16_t* coeffs, int16_t *history, uint16_t n_coeff);

#endif /* IMPLEMENTATION_FIR_H_ */