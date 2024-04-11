#include "hls_stream.h"
#include "ap_int.h"
#include "ap_axi_sdata.h"

#define NUMBER_OF_INPUT_WORDS 467
#define NUMBER_OF_OUTPUT_WORDS 64

#define A_NUM_ROWS 64
#define A_NUM_COLS 7

#define B_NUM_ROWS 8
#define B_NUM_COLS 2
#define B_DISREGARD_BIAS_TERM 2
#define B_OFFSET_FOR_SECOND_NEURON 1
#define HIDDEN_LAYER_FIRST_NEURON 0
#define HIDDEN_LAYER_SECOND_NEURON 1

#define C_NUM_ROWS 3
#define C_NUM_COLS 1
#define C_DISREGARD_BIAS_TERM 1

#define NUM_FRACTIONAL_BITS 8
#define NUM_WEIGHTS_INPUT_TO_HIDDEN 8    // 7(weight connections) + 1(bias)
#define NUM_WEIGHTS_HIDDEN_TO_OUTPUT 3   // 2(weight connections) + 1(bias)
#define NUM_NEURONS_INPUT_LAYER 7
#define NUM_NEURONS_HIDDEN_LAYER 2
#define NUM_NEURONS_OUTPUT_LAYER 1

ap_uint<8> hidden_layer_neurons[NUM_NEURONS_HIDDEN_LAYER][A_NUM_ROWS];
ap_uint<8> output_layer_neurons[A_NUM_ROWS];


// ACLK, ARESETN, TREADY, TDATA, TVALID are essential signals for AXIS.
// TLAST is a sideband signal which is optional in AXIS.
// Rest of the AXI signals are automatically handled by HLS tool.
// However, it is necessary for us since we connecting M_AXIS to AXI Stream FIFO / AXI DMA.

// Declare an AXI-4 Stream interface (without side-channels)
typedef ap_axis<32,0,0,0> AXIS_wLAST;

// https://docs.amd.com/r/en-US/ug1399-vitis-hls/Interfaces-for-Vitis-Kernel-Flow
// https://docs.amd.com/r/en-US/ug1399-vitis-hls/AXI4-Stream-Interfaces
// Since we are using AXI-4 Stream interface protocol, the argument is hls::stream (Paradigm is Stream)
void myip_v1_0_HLS(hls::stream<AXIS_wLAST>& S_AXIS, hls::stream<AXIS_wLAST>& M_AXIS) {

	// https://docs.amd.com/r/en-US/ug1399-vitis-hls/Introduction-to-Interface-Synthesis
	#pragma HLS INTERFACE ap_ctrl_none port=return  // https://docs.amd.com/r/2022.1-English/ug1399-vitis-hls/Using-ap_ctrl_none-Inside-the-Dataflow
													// port=return is what the HLS calls the control interface of synthesized IP block
	#pragma HLS INTERFACE axis port=S_AXIS			// implement port as AXI-4 Stream interface
	#pragma HLS INTERFACE axis port=M_AXIS			// implement port as AXI-4 Stream interface

	// Input matrices maximally 255
	// Output matrix maximally [(255*255)*(A_NUM_COLS)] >> 8
	ap_uint<8> recv_a_matrix[A_NUM_ROWS*A_NUM_COLS] = {0};
	ap_uint<8> recv_b_matrix[B_NUM_ROWS*B_NUM_COLS] = {0};
    ap_uint<8> recv_c_matrix[C_NUM_ROWS*C_NUM_COLS] = {0};

	AXIS_wLAST read_input;
	AXIS_wLAST write_output;

    /**************************** RECEIVE DATA ************************************/
	// We are not making using of S_TLAST (from Master) when Coprocessor (Slave) receives Data
	// S_AXIS_TLAST is required only when we are receiving an unknown number of words.
	myip_v1_0_HLS_receive:for(int word_cnt = 0; word_cnt < NUMBER_OF_INPUT_WORDS; word_cnt++) {
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
        else {
            recv_c_matrix[word_cnt-(A_NUM_ROWS*A_NUM_COLS)-(B_NUM_ROWS*B_NUM_COLS)] = read_input.data;
        }
	}


    /**************************** COMPUTE HIDDEN LAYER ************************************/
    myip_v1_0_HLS_inference_hidden_Layer:for(int i = 0; i < A_NUM_ROWS; i++) {
        ap_uint<32> sum_1 = 0;  // First neuron of hidden layer
        ap_uint<32> sum_2 = 0;  // Second neuron of hidden layer


        // Iterate through 'A_NUM_COLS' features that EACH datapoint has
        for (int j = 0; j < A_NUM_COLS; j++) {
            // Multiply each datapoint feature with the corresponding edge weights
            // Note that we disregard the first row of recv_b_matrix, since that is bias term (for both neurons in the hidden layer), which is NOT multiplied to any feature
            ap_uint<8> datapoint = recv_a_matrix[(i*A_NUM_COLS) + (j)];
            sum_1 += datapoint * recv_b_matrix[B_DISREGARD_BIAS_TERM + (j*NUM_NEURONS_HIDDEN_LAYER)];                              // First neuron of hidden layer
            sum_2 += datapoint * recv_b_matrix[B_DISREGARD_BIAS_TERM + B_OFFSET_FOR_SECOND_NEURON + (j*NUM_NEURONS_HIDDEN_LAYER)]; // Second neuron of hidden layer
        }

        // Include the bias terms now
        sum_1 += recv_b_matrix[HIDDEN_LAYER_FIRST_NEURON];
        sum_2 += recv_b_matrix[HIDDEN_LAYER_SECOND_NEURON];

        // Restore precision, then store the computed weight of our hidden layer neuron
        hidden_layer_neurons[HIDDEN_LAYER_FIRST_NEURON][i] = (sum_1 >> NUM_FRACTIONAL_BITS);
        hidden_layer_neurons[HIDDEN_LAYER_SECOND_NEURON][i] = (sum_2 >> NUM_FRACTIONAL_BITS);
    }


	/**************************** COMPUTE OUTPUT LAYER ************************************/
    // Iterate through 'A_NUM_ROWS' datapoints from BOTH neurons simultaneously
    myip_v1_0_HLS_inference_output_layer:for (int i = 0; i < A_NUM_ROWS; i++) {

        ap_uint<32> sum = 0;

        // Iterate through the weights of output layer, ignoring bias term
        for (int j = 0; j < NUM_WEIGHTS_HIDDEN_TO_OUTPUT-1; j++) {
            sum += hidden_layer_neurons[j][i] * recv_c_matrix[C_DISREGARD_BIAS_TERM + j];
            //sum += sigmoid_function(SOFT_hidden_layer_neurons[j][i]) * recv_c_matrix[C_DISREGARD_BIAS_TERM + j];
        }

        // Include the bias term
        sum += recv_c_matrix[0];

        // Restore precision, then store the computed weight of our output neuron
        // Note output neuron has linear activation function
        output_layer_neurons[i] =  (sum >> NUM_FRACTIONAL_BITS);
    }


	myip_v1_0_HLS_transmit:for(int word_cnt = 0; word_cnt < NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
		// M_TLAST (AXI4-Stream signal) required for connection to AXI DMA
		// M_TLAST is required to be asserted for the last word.
		// Else, the AXI Stream FIFO / AXI DMA will not know if all the words have been received from the co-processor.
		write_output.last = (word_cnt==NUMBER_OF_OUTPUT_WORDS-1) ? 1 : 0;

		// write_output is the element sent by our IP through M_AXIS in one clock cycle.
		write_output.data = output_layer_neurons[word_cnt];

		// write() inserts it into the stream. Overloaded operator << can also be used.
		M_AXIS.write(write_output);
	}

	// De-pulse M_TLAST
	write_output.last = 0;
}