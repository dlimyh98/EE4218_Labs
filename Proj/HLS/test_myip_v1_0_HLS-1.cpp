#include <stdio.h>
#include "hls_stream.h"
#include "ap_axi_sdata.h"

/***************** Macros *********************/
typedef ap_axis<32,0,0,0> AXIS_wLAST;
#define NUMBER_OF_INPUT_WORDS 520
#define NUMBER_OF_OUTPUT_WORDS 64  
#define NUMBER_OF_TEST_VECTORS 1
#define A_NUM_ROWS 64
#define A_NUM_COLS 8
#define B_NUM_ROWS 8
#define B_NUM_COLS 1
#define VERIFICATION_FAIL 1
#define VERIFICATION_PASS 0

/***************** Coprocessor function declaration *********************/
void myip_v1_0_HLS(hls::stream<AXIS_wLAST>& S_AXIS, hls::stream<AXIS_wLAST>& M_AXIS);

/***************** Testbench functions *********************/
void set_expected_memory();
int verify();

/************************** Variable Definitions *****************************/
int test_input_memory [NUMBER_OF_TEST_VECTORS*NUMBER_OF_INPUT_WORDS] = {0xa0,0x81,0x68,0x47,0xb8,0xc1,0x37,0x56,
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


int main()
{
	AXIS_wLAST read_output, write_input;
	hls::stream<AXIS_wLAST> S_AXIS;
	hls::stream<AXIS_wLAST> M_AXIS;

	set_expected_memory();

	for (int test_case_cnt=0 ; test_case_cnt < NUMBER_OF_TEST_VECTORS ; test_case_cnt++) {
		/************************ TRANSMIT DATA TO CO-PROCESSOR **************************/
		printf("TX data, test case %d ... \r\n", test_case_cnt);

		for (int word_cnt=0 ; word_cnt < NUMBER_OF_INPUT_WORDS ; word_cnt++) {
			// S_AXIS_TLAST is asserted for the last word.
			// Actually, doesn't matter since we are not making using of S_AXIS_TLAST.
			write_input.last = (word_cnt==NUMBER_OF_INPUT_WORDS-1) ? 1 : 0;

			write_input.data = test_input_memory[word_cnt+test_case_cnt*NUMBER_OF_INPUT_WORDS];
			S_AXIS.write(write_input); // Insert one word into the stream
		}

		write_input.last = 0;

		/************************ CALL OUR HLS-SYNTHESIZED CO-PROCESSOR **************************/
		myip_v1_0_HLS(S_AXIS, M_AXIS);

		/************************ RECEIVE DATA FROM CO-PROCESSOR **************************/
		printf("RX data, test case %d ... \r\n", test_case_cnt);
		bool is_last = false;
		int word_cnt = 0;

		// Mimic how AXI DMA will look for TLAST
		do {
			read_output = M_AXIS.read();	// Extract one word from stream
			is_last = read_output.last;
			result_memory[word_cnt+test_case_cnt*NUMBER_OF_OUTPUT_WORDS] = read_output.data;
			word_cnt++;
		} while (is_last == false);

		/*
		read_output = M_AXIS.read();	// Extract one word from stream
		for (int word_cnt=0 ; word_cnt < NUMBER_OF_OUTPUT_WORDS ; word_cnt++){
			read_output = M_AXIS.read();    // Extract one word from the stream
			result_memory[word_cnt+test_case_cnt*NUMBER_OF_OUTPUT_WORDS] = read_output.data;
		}
		*/
	}


	if (verify() != 1) {
		printf("Verification failed\n");
		return VERIFICATION_FAIL;
	}
	else {
		printf("Verification success\n");
		return VERIFICATION_PASS;
	}
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


int verify() {
	int success = 1;

	for (int word_cnt=0; word_cnt < NUMBER_OF_TEST_VECTORS*NUMBER_OF_OUTPUT_WORDS; word_cnt++) {
		success = success & (result_memory[word_cnt] == test_result_expected_memory[word_cnt]);
	}

	return success;
}