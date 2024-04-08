#ifndef COMMON_HEADER
    #define COMMON_HEADER
    #include "common.h"
#endif

#include "xstreamer.h"
#include "xllfifo.h"

//#define AXI_STREAM_POLLING_MODE
#define FIFO_DEV_ID             XPAR_AXI_FIFO_0_DEVICE_ID      // AXI_FIFO_MM_S_0 peripheral

int init_base_FIFO_system(u16 FIFODeviceId, XLlFifo* FifoInstancePtr);