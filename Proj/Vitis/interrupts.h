#ifndef COMMON_HEADER
    #define COMMON_HEADER
    #include "common.h"
#endif

#include "xil_exception.h"

#define INTC_DEVICE_ID          XPAR_SCUGIC_SINGLE_DEVICE_ID   // SCUGIC
#define FIFO_INTR_ID            XPAR_FABRIC_LLFIFO_0_VEC_ID    // AXI_FIFO_MM_S_0 fabric interrupt
#define TMRCTR_INTERRUPT_ID     XPAR_FABRIC_TMRCTR_0_VEC_ID    // AXI-Timer Interrupt

#define FIFO_INTERRUPT_PRIORITY      160
#define AXI_TIMER_INTERRUPT_PRIORITY 240
#define RISING_EDGE_SENSIIVE    3
#define NUM_RX_PACKETS_EXPECTED 1