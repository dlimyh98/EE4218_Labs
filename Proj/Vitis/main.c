#include "main.h"

int main()
{
    u32 sw_mult_time = 0;
    u32 hw_mult_time = 0;

    if (initialization() != XST_SUCCESS) {
        xil_printf("Initialization failure\n");
        return XST_FAILURE;
    }

    #ifndef HARD_HLS
        xil_printf("HARD_HDL chosen. AXI-DMA(Polling).\n");
    #else
        xil_printf("HARD_HLS chosen. AXI-Stream(Interrupt).\n");
    #endif

    // Accept files from UART, store data into memory
    xil_printf("Ready to accept files from Realterm\n");
    receive_from_realterm(UART_BASEADDR, recv_a_matrix, recv_b_matrix, test_input_memory);

    set_expected_memory();

    // 1. Load value in TLR0 to TCR0 (by writing to LOAD0)
    // 2. Clear LOAD0, set ENT0 (to let counter run)
    XTmrCtr_Start(TimerCtrInstancePtr, TIMER_CNTR_0);

    do_processing(recv_a_matrix, recv_b_matrix, test_result_expected_memory);

    // Read from TCR0
	sw_mult_time = XTmrCtr_GetValue(TimerCtrInstancePtr, TIMER_CNTR_0);


    #ifndef HARD_HLS
        for (int test_case_cnt = 0; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt++) {
            int Status;

            /*********** TX, Main Memory --> Coprocessor ************/
            Status = mm2s_transmit(&AxiDma, test_case_cnt);
            if (Status != XST_SUCCESS) {
                xil_printf("mm2s TX error\n");
                return XST_FAILURE;
            } 
            //else {
                //xil_printf("mm2s TX success\n");
            //}

            /*********** RX, Coprocessor --> Main Memory ************/
            Status = s2mm_transmit(&AxiDma, test_case_cnt);
            if (Status != XST_SUCCESS) {
                xil_printf("s2mm TX error\n");
                return XST_FAILURE;
            } 
            //else {
                //xil_printf("s2mm TX success\n");
            //}
        }
    #else
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
    #endif


	hw_mult_time = XTmrCtr_GetValue(TimerCtrInstancePtr, TIMER_CNTR_0) - sw_mult_time;
    // TODO: For more accuracy, we can reset Timer0 before counting for HW mult
    xil_printf("SW mult is %d\n", sw_mult_time);
    xil_printf("HW mult is %d", hw_mult_time);

    // Verify results
    return (verify());
}


// Couldn't find a good way to break-up Interrupts and AXI-Stream code into seperate files, they are too interdependent...
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
    if (init_UART(&Uart_Ps) == XST_FAILURE) {
        xil_printf("Failed UART initialization\n");
        return XST_FAILURE;
    }
    override_uart_configs(&Uart_Ps);

    if (init_timer(TimerCtrInstancePtr) == XST_FAILURE) {
        xil_printf("Failed timer initialization\n");
        return XST_FAILURE;
    }

    #ifndef HARD_HLS
        if (init_DMA_system(DMA_DEV_ID, &AxiDma) == XST_FAILURE) {
            xil_printf("Failed DMA initialization\n");
            return XST_FAILURE;
        }
    #else
        if (init_base_FIFO_system(FIFODeviceId, FifoInstancePtr) == XST_FAILURE) {
            xil_printf("Failed base FIFO initialization\n");
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
    #endif

    return XST_SUCCESS;
}

int verify() {
	int success = 1;

	// Compare received HDL/HLS data with our software computation
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