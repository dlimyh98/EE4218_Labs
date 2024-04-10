#include "xparameters.h"
#include "xstatus.h"
#include "xscugic.h"
#include "xil_printf.h"
#include "stdio.h"

#define WORD_SIZE_IN_BYTES 4
#define NUMBER_OF_INPUT_WORDS 467
#define NUMBER_OF_OUTPUT_WORDS 64
#define NUMBER_OF_TEST_VECTORS 1

#define A_NUM_ROWS 64
#define A_NUM_COLS 7

#define B_NUM_ROWS 8
#define B_NUM_COLS 2
#define B_DISREGARD_BIAS_TERM 2
#define B_OFFSET_FOR_SECOND_NEURON 1
#define HIDDEN_LAYER_FIRST_NEURON 0
#define HIDDEN_LAYER_SECOND_NEURON 1

#define C_NUM_ROWS 3
#define C_NUM_COLS 1
#define C_DISREGARD_BIAS_TERM 1