#ifndef COMMON_HEADER
    #define COMMON_HEADER
    #include "common.h"
#endif

#include "xtmrctr.h"

#define TMRCTR_DEVICE_ID        XPAR_TMRCTR_0_DEVICE_ID
#define TIMER_CNTR_0            0

int init_timer(XTmrCtr* TimerCtrInstancePtr);