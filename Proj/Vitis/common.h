#include "xparameters.h"
#include "xstatus.h"
#include "xscugic.h"
#include "xil_printf.h"
#include "stdio.h"

#define WORD_SIZE_IN_BYTES 4
#define NUMBER_OF_INPUT_WORDS 520
#define NUMBER_OF_OUTPUT_WORDS 64
#define NUMBER_OF_TEST_VECTORS 1
#define A_NUM_ROWS 64
#define A_NUM_COLS 8
#define B_NUM_ROWS 8
#define B_NUM_COLS 1
#define TIMEOUT_VALUE 1<<20