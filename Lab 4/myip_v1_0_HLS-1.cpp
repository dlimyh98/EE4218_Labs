#include "hls_stream.h"
#include "ap_int.h"
#include "ap_axi_sdata.h"

#define NUMBER_OF_INPUT_WORDS 520
#define NUMBER_OF_OUTPUT_WORDS 64
#define A_NUM_ROWS 64
#define A_NUM_COLS 8
#define B_NUM_ROWS 8
#define B_NUM_COLS 1

// ACLK, ARESETN, TREADY, TDATA, TVALID are essential signals for AXIS.
// TLAST is a sideband signal which is optional in AXIS.
// However, it is necessary for us since we connecting M_AXIS to AXI Stream FIFO / AXI DMA.

// Declare an AXI-4 Stream interface (without side-channels)
typedef ap_axis<32,0,0,0> AXIS_wLAST;

// https://docs.amd.com/r/en-US/ug1399-vitis-hls/Interfaces-for-Vitis-Kernel-Flow
// https://docs.amd.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces
// Since we are using AXI-4 Stream interface protocol, the argument is hls::stream (Paradigm is Stream)
void myip_v1_0_HLS(hls::stream<AXIS_wLAST>& S_AXIS, hls::stream<AXIS_wLAST>& M_AXIS){

	// https://docs.amd.com/r/en-US/ug1399-vitis-hls/Introduction-to-Interface-Synthesis
	#pragma HLS INTERFACE ap_ctrl_none port=return  // https://docs.amd.com/r/2022.1-English/ug1399-vitis-hls/Using-ap_ctrl_none-Inside-the-Dataflow
													// port=return is what the HLS calls the control interface of synthesized IP block
	#pragma HLS INTERFACE axis port=S_AXIS			// implement port as AXI-4 Stream interface
	#pragma HLS INTERFACE axis port=M_AXIS			// implement port as AXI-4 Stream interface

	// Input matrices maximally 255
	// Output matrix maximally [(255*255)*(A_NUM_COLS)] >> 8
	ap_uint<8> recv_a_matrix[A_NUM_ROWS*A_NUM_COLS] = {0};
	ap_uint<8> recv_b_matrix[B_NUM_ROWS*B_NUM_COLS] = {0};
	ap_uint<12> trans_res_matrix[A_NUM_ROWS*B_NUM_COLS] = {0};

	ap_uint<32> sum = 0;				// (255*255)*(NUM_A_COLS) = 4161600

	AXIS_wLAST read_input;
	AXIS_wLAST write_output;

	// We are not making using of S_TLAST (from Master) when Coprocessor (Slave) receives Data
	// S_AXIS_TLAST is required only when we are receiving an unknown number of words.
	myip_v1_0_HLS_receive:for(int word_cnt = 0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++){
		// read_input is the element (data + other signals) received by our IP through S_AXIS in one clock cycle (which contains one word).
		// read() extracts it from the stream. Overloaded operator >> can also be used.
		read_input = S_AXIS.read();

		if (word_cnt < A_NUM_ROWS*A_NUM_COLS) {
			recv_a_matrix[word_cnt] = read_input.data;	  // Extract the word
		}
		else if (A_NUM_ROWS*A_NUM_COLS <= word_cnt
				&& word_cnt < A_NUM_ROWS*A_NUM_COLS + B_NUM_ROWS*B_NUM_COLS) {
			recv_b_matrix[word_cnt-(A_NUM_ROWS*A_NUM_COLS)] = read_input.data;
		}
	}


	myip_v1_0_HLS_processing_outer:for(int i = 0; i < A_NUM_ROWS; i++) {
		sum = 0;

		myip_v1_0_HLS_processing_inner:for(int j = 0; j < A_NUM_COLS; j++) {
			sum += recv_a_matrix[(i*A_NUM_COLS)+j] * recv_b_matrix[j];
		}

		trans_res_matrix[i] = (sum >> 8);
	}


	myip_v1_0_HLS_transmit:for(int word_cnt = 0; word_cnt < NUMBER_OF_OUTPUT_WORDS; word_cnt++){
		// M_TLAST (AXI4-Stream signal) required for connection to AXI DMA
		// M_TLAST is required to be asserted for the last word.
		// Else, the AXI Stream FIFO / AXI DMA will not know if all the words have been received from the co-processor.
		write_output.last = (word_cnt==NUMBER_OF_OUTPUT_WORDS-1) ? 1 : 0;

		// write_output is the element sent by our IP through M_AXIS in one clock cycle.
		write_output.data = trans_res_matrix[word_cnt];

		// write() inserts it into the stream. Overloaded operator << can also be used.
		M_AXIS.write(write_output);
	}
}