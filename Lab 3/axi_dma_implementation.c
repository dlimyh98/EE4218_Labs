/***************************** Include Files *********************************/
#include "xparameters.h"
#include "xaxidma.h"
#include "xdebug.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xstatus.h"

/***************************** Interrupt or Polling? *********************************/
#define POLLING_MODE

/***************** Macros *********************/
#define DMA_DEV_ID  XPAR_AXIDMA_0_DEVICE_ID    // AXI_DMA_0 peripheral

#define WORD_SIZE_IN_BYTES 4
#define NUMBER_OF_INPUT_WORDS 520
#define NUMBER_OF_OUTPUT_WORDS 64
#define NUMBER_OF_TEST_VECTORS 2
#define A_NUM_ROWS 64
#define A_NUM_COLS 8

/************************** Variable Definitions *****************************/
XAxiDma AxiDma;     // AXI_DMA driver instance

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
0x4b,0x42,0x39,0x00,0x01,0x2e,0x0e,0x25,
0xa0,0x81,0x68,0x47,0xb8,0xc1,0x37,0x56,
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
void set_expected_memory();
int verify();

int init_DMA_system(u16 DeviceId);
int mm2s_transmit(int test_case_cnt);
int s2mm_transmit(int test_case_cnt);


/*****************************************************************************
* Main function
******************************************************************************/
int main()
{
	set_expected_memory();

    if (init_DMA_system(DMA_DEV_ID) == XST_FAILURE) {
        xil_printf("Failed base initialization\n");
        return XST_FAILURE;
    }

    for (int test_case_cnt = 0; test_case_cnt < NUMBER_OF_TEST_VECTORS; test_case_cnt++) {
        int Status;

        /*********** TX, Main Memory --> Coprocessor ************/
        Status = mm2s_transmit(test_case_cnt);
        if (Status != XST_SUCCESS) {
            xil_printf("mm2s TX error\n");
            return XST_FAILURE;
        }

        /*********** RX, Coprocessor --> Main Memory ************/
        Status = s2mm_transmit(test_case_cnt);
        if (Status != XST_SUCCESS) {
            xil_printf("s2mm TX error\n");
            return XST_FAILURE;
        }
    }

    if (verify() != XST_SUCCESS) {
        xil_printf("Verification fail\n");
        return XST_FAILURE;
    }
    else {
        xil_printf("Verification success\n");
        return XST_SUCCESS;
    }
}


int s2mm_transmit(int test_case_cnt) {
    // Tell DMA to do a transfer (note since Stream is source, we need not specify source address)
    int Status = XAxiDma_SimpleTransfer(&AxiDma, (u32)(result_memory+test_case_cnt*NUMBER_OF_OUTPUT_WORDS), 
                        4*NUMBER_OF_OUTPUT_WORDS, XAXIDMA_DEVICE_TO_DMA);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Polling check of DMA's MM2S_DMASR register's Idle flag, but they invert the result
    while (XAxiDma_Busy(&AxiDma,XAXIDMA_DEVICE_TO_DMA)) {
        // Wait for s2mm transfer to complete
    }

    // INVALIDATE the destCache (Main Memory) after receiving the data, so that 
    // PS is forced to read from Main Memory (not cache), which is exactly where Coprocessor wrote to
    Xil_DCacheInvalidateRange((u32)(result_memory+test_case_cnt*NUMBER_OF_OUTPUT_WORDS), 4*NUMBER_OF_OUTPUT_WORDS);

    return XST_SUCCESS;
}


int mm2s_transmit(int test_case_cnt) {
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

    // xil_printf("%p\n", (void*)result_memory);

    // FLUSH the srcCache (Main Memory) and destCache (Coprocessor) before the DMA transfer, so main memory has most recent data
    Xil_DCacheFlushRange((u32)(result_memory+test_case_cnt*NUMBER_OF_INPUT_WORDS), 4*NUMBER_OF_INPUT_WORDS);
    Xil_DCacheFlushRange((u32)(test_input_memory+test_case_cnt*NUMBER_OF_INPUT_WORDS), 4*NUMBER_OF_INPUT_WORDS);

    // Tell DMA to do a transfer (note since Stream is destination, we need not specify destination address)
    int Status = XAxiDma_SimpleTransfer(&AxiDma, (u32)(test_input_memory+test_case_cnt*NUMBER_OF_INPUT_WORDS), 
                        4*NUMBER_OF_INPUT_WORDS, XAXIDMA_DMA_TO_DEVICE);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Polling check of DMA's MM2S_DMASR register's Idle flag, but they invert the result
    while (XAxiDma_Busy(&AxiDma,XAXIDMA_DMA_TO_DEVICE)) {
        // Wait for mm2s transfer to complete
    }

    return XST_SUCCESS;
}


int init_DMA_system(u16 DeviceId) {
	XAxiDma_Config *CfgPtr;
    int Status;

    // Get the config struct of our AXI_DMA device
    CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DeviceId);
		return XST_FAILURE;
	}

    // Initialize a DMA engine
	Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}

    // We should be running DMA in DIRECT REGISTER mode (ie. the simplest mode)
	if (XAxiDma_HasSg(&AxiDma)) {
		xil_printf("DMA configured for Scatter-Gather\r\n");
		return XST_FAILURE;
	}

    // Interrupts
    #ifdef POLLING_MODE
        // Disable DMA interrupts
        XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,
                    XAXIDMA_DEVICE_TO_DMA);
        XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,
                    XAXIDMA_DMA_TO_DEVICE);
    #else
        // Enable DMA interrupts
        asm("NOP");
    #endif

    // For debugging purposes, we might need to disable caches
    //Xil_DCacheDisable();

    return XST_SUCCESS;
}


int verify() {
	int success = 1;

	/* Compare the data send with the data received */
	xil_printf(" Comparing data ...\r\n");

	for (int word_cnt=0; word_cnt < NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
		success = success & (result_memory[word_cnt] == test_result_expected_memory[word_cnt]);

		/*
        if (success != 1) {
            xil_printf("ERR, expect %d got %d\n", test_result_expected_memory[word_cnt], result_memory[word_cnt]);
            break;
        }*/
		xil_printf("%d ", result_memory[word_cnt]);
	}

	if (success != 1){
		return XST_FAILURE;
	}

    return XST_SUCCESS;
}


void set_expected_memory() {
	// A and B are compressed into one array
	int sum = 0;
	int num_elements_summed = 0;
	int B_traversal = 0;

    for (int j = 0; j < NUMBER_OF_TEST_VECTORS; j++) {
        int result_traversal = 0;

        for (int i = 0; i < A_NUM_COLS*A_NUM_ROWS; i++) {
            sum += test_input_memory[(j*NUMBER_OF_INPUT_WORDS)+i] * test_input_memory[(j*NUMBER_OF_INPUT_WORDS)+(A_NUM_ROWS*A_NUM_COLS)+B_traversal];
            B_traversal++;
            num_elements_summed++;

            if (num_elements_summed == A_NUM_COLS) {
                test_result_expected_memory[(j*NUMBER_OF_OUTPUT_WORDS)+result_traversal] = (sum >> 8);
                result_traversal++;

                sum = 0;
                num_elements_summed = 0;
                B_traversal = 0;
            }
        }
    }
}
