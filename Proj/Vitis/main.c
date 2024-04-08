/***************************** Xilinx Includes *********************************/
#include "xil_cache.h"

/***************************** Custom Includes *********************************/
#include "uart.h"
#include "timer.h"
#include "interrupts.h"
#include "axi_stream.h"

/************************** Variable Definitions *****************************/
// UART
XUartPs Uart_Ps;    // Instance of UART Driver. Passed around by functions to refer to SPECIFIC driver instance

// AXI-Stream
u16 FIFODeviceId = FIFO_DEV_ID;
XLlFifo FifoInstance; 	                    // AXIS-FIFO device instance
XLlFifo* FifoInstancePtr = &FifoInstance; 

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
int test_input_memory [NUMBER_OF_TEST_VECTORS*NUMBER_OF_INPUT_WORDS] =  {0xa0,0x81,0x68,0x47,0xb8,0xc1,0x37,0x56,
0x27,0x1f,0x91,0x96,0x20,0x41,0x5a,0x2d,
0x99,0x9f,0x5d,0x18,0x95,0x88,0x6c,0x3d,
0x85,0x49,0xb7,0x7c,0x97,0x27,0x44,0x43,
0x5b,0x18,0x73,0x90,0x7d,0x00,0xff,0x2a,
0x94,0x5c,0x7e,0x47,0xaf,0xcc,0x64,0x9c,
0xaa,0x95,0xa8,0x3a,0xc6,0xc0,0x30,0x56,
0x5b,0x4f,0x2f,0x5b,0x68,0x47,0x92,0x4b,
0x40,0x49,0x97,0x91,0x4e,0x29,0x4e,0x27,
0x60,0x0c,0x7e,0x62,0xb1,0x28,0x76,0x4e,
0x1a,0x6c,0x74,0x81,0x00,0x55,0x36,0x5c,
0x87,0xa7,0xac,0x49,0xa1,0x9f,0x3a,0x40,
0x74,0x68,0xab,0x7c,0x84,0xa6,0x6c,0x48,
0x39,0x40,0x9d,0xb1,0x53,0x47,0x9c,0x16,
0x92,0x0e,0x7f,0x90,0x97,0x5e,0x00,0x2d,
0x8a,0x6f,0x65,0x54,0xaf,0x88,0x30,0x67,
0x89,0x8c,0x6f,0x2d,0x9d,0xff,0x62,0x4b,
0xa1,0x9c,0xac,0x7c,0xaa,0xbd,0x6e,0x7d,
0x45,0x40,0x4d,0x62,0x5c,0xb2,0x88,0x43,
0x97,0xb5,0xb5,0x52,0xaf,0x98,0x5f,0x59,
0x68,0x5a,0xb1,0x6a,0x79,0x80,0x55,0x59,
0x3d,0x19,0x47,0x75,0x81,0x35,0x3f,0x2d,
0xff,0x22,0xff,0xec,0xc0,0x24,0x3c,0x88,
0x5f,0x02,0x76,0x62,0x7c,0x55,0x73,0x32,
0xa6,0xdb,0x9c,0x52,0xfd,0xe1,0x2b,0x78,
0x5c,0xb8,0x6b,0x54,0x78,0x99,0x53,0x43,
0x2b,0x29,0x77,0x62,0x39,0x37,0x44,0x75,
0x89,0x75,0x90,0x83,0x93,0xa5,0x58,0x53,
0x75,0x49,0x7f,0x5b,0x8e,0x95,0x21,0x5c,
0x59,0x6f,0x81,0x89,0xd2,0x3f,0x4b,0x56,
0x53,0x3c,0xd6,0x7c,0x3c,0x31,0x88,0x5c,
0x8c,0x13,0x8d,0x89,0xa2,0x39,0x0f,0x21,
0x56,0x0c,0x72,0x9d,0x53,0x5e,0x5d,0x6a,
0x9c,0xe1,0x81,0x62,0xaa,0xcf,0x8d,0x61,
0x76,0x12,0x6f,0x89,0x8a,0x46,0x2b,0x2a,
0x64,0x8a,0x74,0x47,0x8f,0xc9,0x58,0x48,
0x94,0x29,0x2f,0x3a,0xad,0x8c,0x6c,0x16,
0xb1,0x9f,0x8d,0x0b,0xb8,0xfb,0x8d,0x3a,
0x60,0x00,0x58,0x6f,0x7a,0x41,0xad,0x2d,
0x95,0xe2,0x89,0x70,0xa2,0xac,0x64,0x64,
0xa0,0xd6,0x95,0x3d,0xc8,0xd6,0x5a,0x75,
0x88,0x74,0xc5,0x74,0xaa,0xbf,0x8d,0x67,
0x7c,0x8d,0x6b,0x08,0x8f,0x92,0x5a,0x53,
0x49,0x56,0x51,0x37,0x4a,0x58,0x5d,0xb8,
0x40,0x28,0x82,0xa1,0x3f,0x4c,0xa1,0x38,
0x3a,0x2b,0x83,0x9d,0x33,0x88,0x26,0x2a,
0xb4,0xa7,0x91,0x2d,0xff,0xbc,0x50,0x72,
0x75,0x87,0x65,0x41,0x78,0x89,0x7b,0x4e,
0x53,0x06,0x4f,0x6f,0x74,0x50,0x2b,0x3d,
0x89,0x17,0x74,0x62,0xa1,0x55,0xb7,0x32,
0x98,0xb8,0xb7,0x9d,0xaf,0xb6,0x7d,0x56,
0x78,0x6e,0xcb,0x41,0x8a,0x98,0x64,0x6f,
0xbf,0xff,0x99,0x4e,0xcb,0xd5,0x88,0x6a,
0x00,0x2c,0x00,0x00,0x51,0x5a,0x5a,0x32,
0x53,0x15,0x30,0x47,0x4e,0x87,0x7d,0x2d,
0x76,0x49,0xa4,0x63,0x7a,0x5a,0x7d,0x4e,
0x7b,0xaf,0xb7,0x75,0xaa,0xb8,0x8d,0xad,
0x6f,0xb9,0x95,0x35,0x81,0xe2,0x53,0x48,
0xb0,0xb8,0x9c,0x50,0xd4,0xb5,0x32,0x6a,
0x92,0xb7,0xba,0x7c,0x9d,0xdc,0x5f,0x8b,
0x60,0x78,0x73,0x6f,0x81,0x61,0x76,0xff,
0x7e,0xc8,0xb5,0x56,0xaf,0xb9,0x6e,0x7d,
0x6f,0x50,0x73,0x6c,0x65,0x72,0x76,0x2d,
0x8d,0x90,0x92,0x42,0x9d,0xd8,0x58,0x9f,
0x4b,0x42,0x39,0x00,0x01,0x2e,0x0e,0x25};
int result_memory [NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];
int test_result_expected_memory [NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS];

/************************** Function Definitions *****************************/
int initialization();
void set_expected_memory();
int verify();
int init_interrupts(XScuGic* IntC, XLlFifo* FifoInstancePtr, XTmrCtr* TimerCtrInstancePtr);
static void axi_stream_interrupt_handler (XLlFifo* FifoInstancePtr);
static void timer_interrupt_handler();
int AXIS_transmit(XLlFifo* FifoInstancePtr);
int AXIS_receive(XLlFifo* FifoInstancePtr);


/*****************************************************************************
* Main function
******************************************************************************/
int main()
{
    u32 sw_mult_time = 0;
    u32 hw_mult_time = 0;

    if (initialization() != XST_SUCCESS) {
        xil_printf("Initialization failure\n");
        return XST_FAILURE;
    }

    // Accept files from UART, do SW processing
    xil_printf("Ready to accept files from Realterm\n");
    receive_from_realterm(UART_BASEADDR, recv_a_matrix, recv_b_matrix);
    xil_printf("Do SW processing\n");

    // 1. Load value in TLR0 to TCR0 (by writing to LOAD0)
    // 2. Clear LOAD0, set ENT0 (to let counter run)
    XTmrCtr_Start(TimerCtrInstancePtr, TIMER_CNTR_0);

    do_processing(recv_a_matrix, recv_b_matrix, test_result_expected_memory);

    // Read from TCR0
	sw_mult_time = XTmrCtr_GetValue(TimerCtrInstancePtr, TIMER_CNTR_0);

    // Communicate to Coprocessor IP via AXI-Stream
	for (; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt++) {
        /********************* TX *********************/
        if (AXIS_transmit(FifoInstancePtr) != XST_SUCCESS) {
            xil_printf("TX error\n");
            return XST_FAILURE;
        }

        // Interrupt mode: Do other work while waiting for TX completion
        #ifndef AXI_STREAM_POLLING_MODE
            while (!TX_done) {
                asm("nop");
                //xil_printf("Busy TX\n");
            }
        #endif

        /********************* RX *********************/
        // Polling mode: Polling read of RDRO register
        // Interrupt mode: Only read RDRO register when RC flag is raised
        if (AXIS_receive(FifoInstancePtr) != XST_SUCCESS) {
            xil_printf("RX error\n");
            return XST_FAILURE;
        }
	}

	hw_mult_time = XTmrCtr_GetValue(TimerCtrInstancePtr, TIMER_CNTR_0) - sw_mult_time;
    // TODO: For more accuracy, we can reset Timer0 before counting for HW mult
    xil_printf("SW mult is %d\n", sw_mult_time);
    xil_printf("HW mult is %d", hw_mult_time);

    // Verify results
    return (verify());
}


// Couldn't find a good way to break-up Interrupt and AXI-Stream code into seperate files, they are too interdependent...
/*********************************** Interrupt *********************************************/
static void axi_stream_interrupt_handler (XLlFifo* FifoInstancePtr) {
    // Check which interrupt source was triggered
    u32 Pending = XLlFifo_IntPending(FifoInstancePtr);

    // Clear ALL interrupts (there may be back-to-back interrupts)
    while (Pending) {
        if (Pending & XLLF_INT_TC_MASK) {
            TX_done = 1;
            XLlFifo_IntClear(FifoInstancePtr, XLLF_INT_TC_MASK);
        }
        else if (Pending & XLLF_INT_RC_MASK) {
            xil_printf("ISR's RC triggered\n");

            /* ISR's RC flag is raised
                - Indicates that at least one successful receive of packet(s) has completed
                - Received packet's data and length are available
                - Note this interrupt can represent MORE than one packet received
                    We must check FIFO's RX occupancy to determine if we have additional received packets for processing
            */

            // Check the number of words (32-bit sized in our case) avail from FIFO's RX
            // It tells is the number of locations in use for data storage in the FIFO's RX, after the most recent transaction
            // Note this value is only updated after a packet is SUCCESSFULLY received
            // Reads from RDRO register, https://docs.xilinx.com/r/en-US/pg080-axi-fifo-mm-s/Interrupt-Status-Register-ISR
            while (XLlFifo_iRxOccupancy(FifoInstancePtr)) {
                // We are expecting only one packet from testbench per testcase
                u32 received_length = XLlFifo_iRxGetLen(FifoInstancePtr);

                for (int word_cnt=0; word_cnt < received_length/WORD_SIZE_IN_BYTES; word_cnt++) {
                        u32 RxWord = XLlFifo_RxGetWord(FifoInstancePtr);
                        result_memory[word_cnt+test_case_cnt*NUMBER_OF_OUTPUT_WORDS] = RxWord;
                }
            }

            packets_received++;
            XLlFifo_IntClear(FifoInstancePtr, XLLF_INT_RC_MASK);
        }
        else {
            // Shouldn't come here actually
            XLlFifo_IntClear(FifoInstancePtr, Pending);
        }

        // Check if any other queued interrupts
		Pending = XLlFifo_IntPending(FifoInstancePtr);
    }
}

static void timer_interrupt_handler() {
    xil_printf("Pass\n");
}

int init_interrupts(XScuGic* IntC, XLlFifo* FifoInstancePtr, XTmrCtr* TimerCtrInstancePtr) {
    /* https://support.xilinx.com/s/article/763748?language=en_US
       1. In the IP block diagram, connect AXI-Stream FIFO's interrupt to Zynq MPSoC pl_ps_irq0[0:0]
            - This interrupt belongs to the MPSoC's APU interrupts, specifically it is Interrupt Group 0
            - Interrupts within Group 0 may be assigned to IRQ or FIQ (assignments occur internally within GICv2)

       2. Initializing interrupts is application specific
            - AXIS FIFO could be connected to processor with/without interrupt controller
            - For MPSoC, I think it has an interrupt controller
    */
   int Status;

   // Configuration for SCUGIC (System Controller Ultra Generic Interrupt Controller)
   XScuGic_Config* IntcConfig;

   // Lookup config data for the SCUGIC IP-core
   IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
   if (IntcConfig == NULL) return XST_FAILURE;

   // Initialize our SPECIFIC interrupt controller
   Status = XScuGic_CfgInitialize(IntC, IntcConfig,
                             IntcConfig->CpuBaseAddress);
   if (Status != XST_SUCCESS) return XST_FAILURE;

   // Sets priority/trigger types for our specified IRQ sources
   #ifndef AXI_STREAM_POLLING_MODE
        XScuGic_SetPriorityTriggerType(IntC, (u16)FIFO_INTR_ID,
                                    FIFO_INTERRUPT_PRIORITY, RISING_EDGE_SENSIIVE);
   #endif
   XScuGic_SetPriorityTriggerType(IntC, (u16) TMRCTR_INTERRUPT_ID,
                            AXI_TIMER_INTERRUPT_PRIORITY, RISING_EDGE_SENSIIVE);

    // Connect our interrupt handlers
    #ifndef AXI_STREAM_POLLING_MODE
        Status = XScuGic_Connect(IntC, (u16)FIFO_INTR_ID,
                    (Xil_InterruptHandler)axi_stream_interrupt_handler, FifoInstancePtr);
        if (Status != XST_SUCCESS) {
            xil_printf("Fail to connect AXIS-FIFO interrupt handler\n");
            return Status;
        }
    #endif
	Status = XScuGic_Connect(IntC, (u16)TMRCTR_INTERRUPT_ID,
				(Xil_InterruptHandler)timer_interrupt_handler, TimerCtrInstancePtr);
    if (Status != XST_SUCCESS) {
        xil_printf("Fail to connect AXI-Timer interrupt handler\n");
        return Status;
    }

    // Enable interrupts
    #ifndef AXI_STREAM_POLLING_MODE
        XScuGic_Enable(IntC, (u16)FIFO_INTR_ID);
    #endif
    //TODO: TMRCTR Interrupt logic not implemented yet
    //XScuGic_Enable(IntC, (u16)TMRCTR_INTERRUPT_ID);

    // Initialize the exception table
    Xil_ExceptionInit();

    // Register our interrupt controller handler with the exception table
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
		(Xil_ExceptionHandler)XScuGic_InterruptHandler,
		(void*)IntC);

    // Enable exceptions
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

/*********************************** AXI-Stream TX,RX *********************************************/
int AXIS_transmit(XLlFifo* FifoInstancePtr) {
    int word_cnt = 0;

    /******************** Input to Coprocessor : Transmit the Data Stream ***********************/
    //xil_printf(" Transmitting Data for test case %d ... \r\n", test_case_cnt);

    // Writing into the FIFO Transmit Port Buffer
    for (word_cnt=0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++) {
        if( XLlFifo_iTxVacancy(FifoInstancePtr) ){
            // We set AXIS FIFO depth to 1024 (words) in Vivado, so it can comfortably fit NUMBER_OF_INPUT_WORDS
            XLlFifo_TxPutWord(FifoInstancePtr, test_input_memory[word_cnt+test_case_cnt*NUMBER_OF_INPUT_WORDS]);
        }
    }

    // Kickoff transmission by declaring transmission length (in bytes)
    XLlFifo_iTxSetLen(FifoInstancePtr, WORD_SIZE_IN_BYTES*NUMBER_OF_INPUT_WORDS);

    #ifdef AXI_STREAM_POLLING_MODE
        // POLLING check for TX completion, by checking the TC flag of ISR register
        // How is this different from Interrupt mode? In Polling, the corresponding IER is NOT asserted
        // Thus we need to keep polling the TC flag
        while( !(XLlFifo_IsTxDone(FifoInstancePtr)) ) {}
        /* Transmission Complete */
        return XST_SUCCESS;
    #else
        // When transmission completes, it will raise an interrupt
        // Let our interrupt-handler settle it
        return XST_SUCCESS;
    #endif
}

int AXIS_receive(XLlFifo* FifoInstancePtr) {
    #ifdef AXI_STREAM_POLLING_MODE
        /******************** Output from Coprocessor : Receive the Data Stream ***********************/
        xil_printf(" Receiving data for test case %d ... \r\n", test_case_cnt);

        int timeout_count = TIMEOUT_VALUE;
        // Check the number of words (32-bit sized in our case) avail from FIFO's RX, subject to a timeout
        // It tells is the number of locations in use for data storage in the FIFO's RX, after the most recent transaction
        // Note this value is only updated after a packet is SUCCESSFULLY received
        // Reads from RDRO register, https://docs.xilinx.com/r/en-US/pg080-axi-fifo-mm-s/Interrupt-Status-Register-ISR
        while(!XLlFifo_iRxOccupancy(FifoInstancePtr)) {
            timeout_count--;
            if (timeout_count == 0)
            {
                xil_printf("Timeout while waiting for data ... \r\n");
                return XST_FAILURE;
            }
        }

        // For AXIS, one PACKET = sequence of DATA until TLAST
        // https://docs.xilinx.com/v/u/4.1-English/pg080-axi-fifo-mm-s -- Pg14, axi_str_rxd_tlast --> TLAST: Indicates boundary of a packet
        // Thus, we expect only one PACKET of data per test case

        // If more packets are expected from the coprocessor, the part below should be done in a loop.
        // https://docs.xilinx.com/r/en-US/pg080-axi-fifo-mm-s/Receive-Length-Register-RLR
        u32 num_bytes_in_packet = XLlFifo_iRxGetLen(FifoInstancePtr);    // Reads from RLR register

        // Read one word at a time
        for (int word_cnt=0; word_cnt < num_bytes_in_packet/4; word_cnt++) {
            result_memory[word_cnt+test_case_cnt*NUMBER_OF_OUTPUT_WORDS] = XLlFifo_RxGetWord(FifoInstancePtr);
        }

        int Status = XLlFifo_IsRxDone(FifoInstancePtr);
        if(Status != TRUE){
            xil_printf("Failing in receive complete ... \r\n");
            return XST_FAILURE;
        }

        return XST_SUCCESS;
        /* Reception Complete */
    #else
        while (packets_received != NUM_RX_PACKETS_EXPECTED) {
            //asm("nop");
            xil_printf("Busy RX\n");
        }

        return XST_SUCCESS;
    #endif
}

/********************************** Generic *********************************************/
int initialization() {
    set_expected_memory();

    if (init_UART(&Uart_Ps) == XST_FAILURE) {
        xil_printf("Failed UART initialization\n");
        return XST_FAILURE;
    }
    override_uart_configs(&Uart_Ps);

    if (init_base_FIFO_system(FIFODeviceId, FifoInstancePtr) == XST_FAILURE) {
        xil_printf("Failed base FIFO initialization\n");
        return XST_FAILURE;
    }

    if (init_timer(TimerCtrInstancePtr) == XST_FAILURE) {
        xil_printf("Failed timer initialization\n");
        return XST_FAILURE;
    }

    #ifndef AXI_STREAM_POLLING_MODE
        if (init_interrupts(&IntC, FifoInstancePtr, TimerCtrInstancePtr) != XST_SUCCESS) {
            xil_printf("Failed interrupt initialization\n");
            return XST_FAILURE;
        } else {
            // Enable AXIS_FIFO with choice of interrupts
            XLlFifo_IntEnable(FifoInstancePtr, XLLF_INT_TC_MASK|XLLF_INT_RC_MASK);
        }
    #endif

    return XST_SUCCESS;
}

int verify() {
	int success = 1;

	// Compare the data send with the data received
	xil_printf(" Comparing data ...\r\n");
	for (int word_cnt=0; word_cnt < NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
		success = success & (result_memory[word_cnt] == test_result_expected_memory[word_cnt]);
	}

	if (success != 1){
        xil_printf("Verification fail\n");
		return XST_FAILURE;
	}

    xil_printf("Verification success\n");
    return XST_SUCCESS;
}

void set_expected_memory() {
	// A and B are compressed into one array
	int sum = 0;
	int num_elements_summed = 0;
	int B_traversal = 0;
	int result_traversal = 0;

	for (int i = 0; i < A_NUM_COLS*A_NUM_ROWS; i++) {
		sum += test_input_memory[i] * test_input_memory[(A_NUM_ROWS*A_NUM_COLS)+B_traversal];
        B_traversal++;
		num_elements_summed++;

		if (num_elements_summed == A_NUM_COLS) {
			test_result_expected_memory[result_traversal] = (sum >> 8);
			result_traversal++;

			sum = 0;
			num_elements_summed = 0;
            B_traversal = 0;
		}
	}
}