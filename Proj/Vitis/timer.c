#include "timer.h"


int init_timer(XTmrCtr* TimerCtrInstancePtr) {
    // NOTE: Only Timer0 is enabled in Vivado block diagram
    int status;

    // Baseline initialization of AXI-Timer. Namely...
    // 1. TLR0 (Load Register) set to 0
    // 2. Timer0 set to GENERATE mode, and up-counting
    status = XTmrCtr_Initialize(TimerCtrInstancePtr, TMRCTR_DEVICE_ID);
    if (status != XST_SUCCESS) {
        xil_printf("Error in baseline initialization of AXI-Timer\n");
        return status;
    }

    // Enable Autoreload mode (ARHT0) of timer counter
    // GENERATE mode Timer will RELOAD value from Load Register (0) when a carryout occurs
    // Not really important to us
	XTmrCtr_SetOptions(TimerCtrInstancePtr, TIMER_CNTR_0, XTC_AUTO_RELOAD_OPTION);

    return XST_SUCCESS;
}