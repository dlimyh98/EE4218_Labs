#include "axi_dma.h"

int s2mm_transmit(XAxiDma* AxiDma, int test_case_cnt) {
    // Tell DMA to do a transfer (note since Stream is source, we need not specify source address)
    int Status = XAxiDma_SimpleTransfer(AxiDma, (u32)(HARD_result_memory+test_case_cnt*NUMBER_OF_OUTPUT_WORDS), 
                        4*NUMBER_OF_OUTPUT_WORDS, XAXIDMA_DEVICE_TO_DMA);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Check receive channel, it should be running after SimpleTransfer()
    /*
	XAxiDma_BdRing* RxRingPtr = XAxiDma_GetRxRing(AxiDma);
	int register_value = XAxiDma_ReadReg(RxRingPtr->ChanBase,
						XAXIDMA_CR_OFFSET);
    xil_printf("%0d\n", register_value & XAXIDMA_CR_RUNSTOP_MASK);
    */

    // Polling check of DMA's S2MM_DMASR register's Idle flag, but they invert the result
    while (XAxiDma_Busy(AxiDma,XAXIDMA_DEVICE_TO_DMA)) {
        // Wait for s2mm transfer to complete

        // TODO: Stuck here when using HLS generated IP
        /*
        int value = ((XAxiDma_ReadReg((AxiDma)->RegBase +
				 (XAXIDMA_RX_OFFSET * XAXIDMA_DEVICE_TO_DMA),
				 XAXIDMA_SR_OFFSET) &
		 XAXIDMA_HALTED_MASK));
         xil_printf("%0d\n", value);
         */
    }


    // INVALIDATE the destCache (Main Memory) after receiving the data, so that 
    // PS is forced to read from Main Memory (not cache), which is exactly where Coprocessor wrote to
    Xil_DCacheInvalidateRange((u32)(HARD_result_memory+test_case_cnt*NUMBER_OF_OUTPUT_WORDS), 4*NUMBER_OF_OUTPUT_WORDS);

    return XST_SUCCESS;
}


int mm2s_transmit(XAxiDma* AxiDma, int test_case_cnt) {
    // DMA (MASTER) reads data from main memory, and transmit to Coprocessor (SLAVE)

    // Note that the DMA driver example uses hardcoded address for memory
    /* Lets examine the example driver code
        - It has a TxBufferPtr pointer to a TX_BUFFER_BASE address
        - TxBufferPointer is where we store the data (in main memory), to be sent to Coprocessor
        - TxBufferPointer is the start address of the range to be invalidated in Cache

        // https://docs.xilinx.com/r/en-US/ug1085-zynq-ultrascale-trm/System-Address-Map-Interconnects
        // Viewing the lscript in Vitis, we see that psu_ddr_0_MEM_0 --> 0x0000_0000 - 0x7FF00_0000, it is DDR low in System Map
        //                                           psu_ddr_1_MEM_0 --> 0x8_0000_0000 - 0x10_0000_0000, it is DDR high in System Map

        - TX_BUFFER_BASE is defined as (MEM_BASE_ADDR + 0x0010_0000)
        - MEM_BASE_ADDR is defined as (DDR_BASE_ADDR + 0x0100_0000)
        - DDR_BASE_ADDR Is defined as XPAR_PSU_DDR_0_S_AXI_BASEADDR (0x0)
        - Thus, TX_BUFFER_BASE is 0x0110_0000, within our psu_ddr_0_MEM_0 region

        - RX_BUFFER_BASE is defined as (MEM_BASE_ADDR + 0x0030_0000)
        - Thus, RX_BUFFER_BASE is 0x0130_0000, within our psu_ddr_0_MEM_0 region
    */
    // xil_printf("%p\n", (void*)HARD_result_memory);

    // FLUSH the srcCache (Main Memory) and destCache (Coprocessor) before the DMA transfer, so main memory has most recent data
    Xil_DCacheFlushRange((u32)(HARD_result_memory+test_case_cnt*NUMBER_OF_INPUT_WORDS), 4*NUMBER_OF_INPUT_WORDS);
    Xil_DCacheFlushRange((u32)(HARD_input_memory+test_case_cnt*NUMBER_OF_INPUT_WORDS), 4*NUMBER_OF_INPUT_WORDS);

    // Tell DMA to do a transfer (note since Stream is destination, we need not specify destination address)
    int Status = XAxiDma_SimpleTransfer(AxiDma, (u32)(HARD_input_memory+test_case_cnt*NUMBER_OF_INPUT_WORDS), 
                        4*NUMBER_OF_INPUT_WORDS, XAXIDMA_DMA_TO_DEVICE);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Polling check of DMA's MM2S_DMASR register's Idle flag, but they invert the result
    while (XAxiDma_Busy(AxiDma,XAXIDMA_DMA_TO_DEVICE)) {
        // Wait for mm2s transfer to complete
    }

    // Try turning off the DMA's MM2S channel
    /*
    XAxiDma_WriteReg((AxiDma)->TxBdRing.ChanBase, XAXIDMA_CR_OFFSET, 0);

	XAxiDma_BdRing* TxRingPtr = XAxiDma_GetTxRing(AxiDma);
	int register_value = XAxiDma_ReadReg(TxRingPtr->ChanBase,
						XAXIDMA_CR_OFFSET);
    */
    return XST_SUCCESS;
}

int init_DMA_system(u16 DeviceId, XAxiDma* AxiDma) {
	XAxiDma_Config *CfgPtr;
    int Status;

    // Get the config struct of our AXI_DMA device
    CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

    // Initialize a DMA engine
	Status = XAxiDma_CfgInitialize(AxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

    // We should be running DMA in DIRECT REGISTER mode (ie. the simplest mode)
	if (XAxiDma_HasSg(AxiDma)) {
		xil_printf("DMA configured for Scatter-Gather\r\n");
		return XST_FAILURE;
	}

    // Disable DMA Interrupts
    XAxiDma_IntrDisable(AxiDma, XAXIDMA_IRQ_ALL_MASK,
                XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(AxiDma, XAXIDMA_IRQ_ALL_MASK,
                XAXIDMA_DMA_TO_DEVICE);

    // For debugging purposes, we might need to disable caches
    //Xil_DCacheDisable();

    return XST_SUCCESS;
}