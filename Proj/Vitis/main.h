/********************************* DEFINES *************************************/
// Choose between 
//  - AXI-DMA (Polling) connected HDL; (HARD_HDL)
//  - AXI-Stream (Interrupt) connected HLS (HARD_HLS)
#define HARD_HLS

#define NUM_WEIGHTS_INPUT_TO_HIDDEN 8    // 7(weight connections) + 1(bias)
#define NUM_WEIGHTS_HIDDEN_TO_OUTPUT 3   // 2(weight connections) + 1(bias)
#define NUM_NEURONS_INPUT_LAYER 7
#define NUM_NEURONS_HIDDEN_LAYER 2
#define NUM_NEURONS_OUTPUT_LAYER 1

/******************************** INCLUDES *************************************/
#include "uart.h"
#include "timer.h"
#include "interrupts.h"
#include "axi_stream.h"
#include "axi_dma.h"

/******************************* VARIABLES *************************************/
// UART
XUartPs Uart_Ps;    // Instance of UART Driver. Passed around by functions to refer to SPECIFIC driver instance

// AXI-Stream
u16 FIFODeviceId = FIFO_DEV_ID;
XLlFifo FifoInstance; 	                    // AXIS-FIFO device instance
XLlFifo* FifoInstancePtr = &FifoInstance; 

// AXI-DMA
XAxiDma AxiDma;     // AXI_DMA driver instance

// Timer
XTmrCtr TimerCounterInst;                   // AXI-Timer device instance
XTmrCtr* TimerCtrInstancePtr = &TimerCounterInst;

// Interrupts
static XScuGic IntC;                        // Interrupt Controller instance
volatile int TX_done = 0;
volatile int packets_received = 0;

// main.c
char recv_a_matrix[A_NUM_ROWS*A_NUM_COLS] = {0};
char recv_b_matrix[B_NUM_ROWS*B_NUM_COLS] = {0};
int trans_res_matrix[A_NUM_ROWS*B_NUM_COLS] = {0};

int test_case_cnt = 0;
int test_input_memory[NUMBER_OF_TEST_VECTORS*NUMBER_OF_INPUT_WORDS];
int result_memory[NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];
int test_result_expected_memory[NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];

// Suppose f(x) describes sigmoid function, and x is in Q<0.8> format.
// Suppose we scale up x to Q<8.0> format.
// Then applying sigmoid definition, store sigmoid output as (2^8) LUT entries, EACH as Q<8.0> uint8.
// Of course we can store more than (2^8) LUT entries for greater precision, just chose 256 for convenience.
u8 sigmoid_LUT[256] = {12,12,12,12,13,13,13,14,14,14,15,15,15,16,16,16,
                        17,17,18,18,18,19,19,20,20,21,21,21,22,22,23,23,
                        24,24,25,26,26,27,27,28,28,29,30,30,31,32,32,33,
                        34,34,35,36,36,37,38,39,39,40,41,42,43,44,44,45,
                        46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,
                        62,63,64,66,67,68,69,70,72,73,74,75,76,78,79,80,
                        82,83,84,86,87,88,90,91,92,94,95,97,98,99,101,102,
                        104,105,107,108,110,111,113,114,116,117,119,120,122,123,125,126,
                        128,129,130,132,133,135,136,138,139,141,142,144,145,147,148,150,
                        151,153,154,156,157,158,160,161,163,164,165,167,168,169,171,172,
                        173,175,176,177,179,180,181,182,183,185,186,187,188,189,191,192,
                        193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,
                        209,210,211,211,212,213,214,215,216,216,217,218,219,219,220,221,
                        221,222,223,223,224,225,225,226,227,227,228,228,229,229,230,231,
                        231,232,232,233,233,234,234,234,235,235,236,236,237,237,237,238,
                        238,239,239,239,240,240,240,241,241,241,242,242,242,243,243,243};

u8 weights_hidden[NUM_WEIGHTS_INPUT_TO_HIDDEN*NUM_NEURONS_HIDDEN_LAYER] = {26,6,
                                                                           25,18,
                                                                           31,6,
                                                                           29,26,
                                                                           22,1,
                                                                           1,28,
                                                                           11,9,
                                                                           26,45};

u8 weights_output[NUM_WEIGHTS_HIDDEN_TO_OUTPUT*NUM_NEURONS_OUTPUT_LAYER] = {80,50,200};

/******************************* FUNCTION DECLARATIONS *************************************/
int initialization();
void set_expected_memory();
int verify();
int init_interrupts(XScuGic* IntC, XLlFifo* FifoInstancePtr, XTmrCtr* TimerCtrInstancePtr);
static void axi_stream_interrupt_handler (XLlFifo* FifoInstancePtr);
static void timer_interrupt_handler();
int AXIS_transmit(XLlFifo* FifoInstancePtr);
int AXIS_receive(XLlFifo* FifoInstancePtr);