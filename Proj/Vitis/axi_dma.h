#ifndef COMMON_HEADER
    #define COMMON_HEADER
    #include "common.h"
#endif

#include "xaxidma.h"
#include "xil_cache.h"

#define DMA_DEV_ID  XPAR_AXIDMA_0_DEVICE_ID    // AXI_DMA_0 peripheral

extern int test_case_cnt;
extern int HARD_input_memory [NUMBER_OF_TEST_VECTORS*NUMBER_OF_INPUT_WORDS];
extern int HARD_result_memory [NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];

int init_DMA_system(u16 DeviceId, XAxiDma* AxiDma);
int mm2s_transmit(XAxiDma* AxiDma, int test_case_cnt);
int s2mm_transmit(XAxiDma* AxiDma, int test_case_cnt);