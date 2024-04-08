#include "axi_stream.h"

int init_base_FIFO_system(u16 FIFODeviceId, XLlFifo* FifoInstancePtr) {
	int Status;
	XLlFifo_Config *Config;

	/* Initialize the Device Configuration Interface driver */
	Config = XLlFfio_LookupConfig(FIFODeviceId);
	if (!Config) {
		xil_printf("No config found for %d\r\n", FIFODeviceId);
		return XST_FAILURE;
	}

	Status = XLlFifo_CfgInitialize(FifoInstancePtr, Config, Config->BaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed\r\n");
		return XST_FAILURE;
	}

	/* Check for the Reset value */
	Status = XLlFifo_Status(FifoInstancePtr);
	XLlFifo_IntClear(FifoInstancePtr,0xffffffff);
	Status = XLlFifo_Status(FifoInstancePtr);
	if(Status != 0x0) {
		xil_printf("\n ERROR : Reset value of ISR0 : 0x%x\t. Expected : 0x0\r\n",
			    XLlFifo_Status(FifoInstancePtr));
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}