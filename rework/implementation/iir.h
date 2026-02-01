#ifndef IIR_H
#define IIR_H

#include "stdint.h"

int16_t second_order_IIR( int16_t input, int16_t* coefficients, int16_t* x_history, int16_t* y_history);

int16_t fourth_order_IIR(int16_t input, int16_t coefficients[][6], int16_t x_history[][2], int16_t y_history[][2]);
		
int16_t sixth_order_IIR(int16_t input, int16_t coefficients[][6], int16_t x_history[][2], int16_t y_history[][2]);

#endif
