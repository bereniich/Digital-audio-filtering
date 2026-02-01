#include "iir.h"
#include "conv.h"
 
int16_t second_order_IIR(int16_t input, int16_t* coefficients, int16_t* x_history, int16_t* y_history) {
    // coefficients: [a0, a1/2, a2, b0, b1/2, b2] 
    int32_t acc = 0;

    // FIR part: a0*x[n] + a1*x[n-1] + a2*x[n-2]
    acc = (int32_t)input * coefficients[0];                                 // a0*x[n]
    acc = signed_multipy_accumulate(acc, x_history[0], coefficients[1]);    // a1*x[n-1]
    acc += (int32_t)x_history[1] * coefficients[2];                         // a2*x[n-2]

    // IIR part: -b1*y[n-1] - b2*y[n-2]
    acc = signed_multipy_subtract(acc, y_history[0], coefficients[4]);      // -b1*y[n-1]
    acc -= (int32_t)y_history[1] * coefficients[5];                         // -b2*y[n-2]

    // Scaling
    int16_t output = (int16_t)(acc >> 15);

    // Update history
    x_history[1] = x_history[0]; 
    x_history[0] = input;
    y_history[1] = y_history[0];
    y_history[0] = output;

    return output;
}


int16_t fourth_order_IIR(int16_t input, int16_t coefficients[][6], int16_t x_history[][2], int16_t y_history[][2]) {
    /* steps are the same as in second order iir but are done in three stages */

    int16_t stage_one = second_order_IIR(input, coefficients[0], x_history[0], y_history[0]);
    int16_t stage_two = second_order_IIR(stage_one, coefficients[1], x_history[1], y_history[1]);
    
    return stage_two;
}


int16_t sixth_order_IIR(int16_t input, int16_t coefficients[][6], int16_t x_history[][2], int16_t y_history[][2]) {
    /* steps are the same as in second order iir but are done in three stages */

    int16_t stage_one = second_order_IIR(input, coefficients[0], x_history[0], y_history[0]);
    int16_t stage_two = second_order_IIR(stage_one, coefficients[1], x_history[1], y_history[1]);
    int16_t stage_three = second_order_IIR(stage_two, coefficients[2], x_history[2], y_history[2]);
    
    return stage_three;
}

