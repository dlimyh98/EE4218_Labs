#include "uart.h"

int init_UART(XUartPs* Uart_Ps_ptr) {
    // 1. For our device, retrieve a pointer to it's config struct
	XUartPs_Config *Config;

	Config = XUartPs_LookupConfig(UART_DEVICE_ID);
    if (Config == NULL) {
        return XST_FAILURE;
    }

    // 2. Initialize the configured device (to default values), using the Config pointer
    int Status;
	Status = XUartPs_CfgInitialize(Uart_Ps_ptr, Config, Config->BaseAddress);
    if (Status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    return XST_SUCCESS;
}


void override_uart_configs(XUartPs* Uart_Ps_ptr) {
	XUartPs_SetBaudRate(Uart_Ps_ptr, MY_BAUD_RATE);
}


void receive_from_realterm(u32 uart_base_addr, char* recv_a_matrix, char* recv_b_matrix, char* recv_c_matrix, int* HARD_input_memory) {
    // Data is sent through .csv files via Realterm
    // .csv files MUST be in Unix format (i.e consider line break as 0xA). Can use Vim to set fileformat to Unix.
    // Also note that the file MUST have EOL character. Can use Vim to check also.

    int valid_recv_count = 0;    // Number of elements in our usual Matrix

    char buffer[CONCAT_BUFFER_SIZE] = {0};  // Maximal incoming matrix value is '255', formed by 3 chars
    int num_insertions = 0;

    while(1) {
        // Check if all valid data has been received
        // Note that we need to check BEFORE we begin RX polling
        if (valid_recv_count == (A_NUM_ROWS*A_NUM_COLS)
                              + (B_NUM_ROWS*B_NUM_COLS)
                              + (C_NUM_ROWS*C_NUM_COLS)) return;

        // Polling until any data is received
        while (!XUartPs_IsReceiveData(uart_base_addr));

        // Read from Transmit and Receive FIFO[7:0]
        u8 recv_char = XUartPs_ReadReg(uart_base_addr, XUARTPS_FIFO_OFFSET);

        // Parse the data accordingly
        if (recv_char == NEW_LINE || recv_char == COMMA) {
            // Newline, means we are going to 'next row' of matrix
            // Comma, means we are transitioning to next matrix 'element'
            u8 concat_char = concat_char_buffer(buffer, num_insertions-1);

            // Concat all data into one array for sending over using DMA (in ONE transaction)
            *HARD_input_memory = concat_char;
            HARD_input_memory++;

            // Split incoming data into A,B,C matrix
            if (valid_recv_count < A_NUM_ROWS*A_NUM_COLS) {
                *recv_a_matrix = concat_char;
                recv_a_matrix++;
            } 
            else if (A_NUM_ROWS*A_NUM_COLS <= valid_recv_count 
                    && valid_recv_count < A_NUM_ROWS*A_NUM_COLS + B_NUM_ROWS*B_NUM_COLS) {
                *recv_b_matrix = concat_char;
                recv_b_matrix++;
            }
            else {
                *recv_c_matrix = concat_char;
                recv_c_matrix++;
            }

            // Book-keeping before continuing to next loop
            valid_recv_count++;
            num_insertions = 0;
        }
        else if (0x30 <= recv_char && recv_char <= 0x39) {
            // Received VALID char that forms up matrix element
            // e.g integer '46' is formed from chars '4' , '6'
            buffer[num_insertions] = recv_char;
            num_insertions++;
        }
        else {
            // Some invalid character, print error
            xil_printf("DETECTED INVALID CHARACTER %c\n", recv_char);
        }

    }
}


void send_to_realterm(u32 uart_base_address, int* trans_res_matrix) {
    // Additional +1 to accomodate null-termination of strings
    // Note string literals must be char, NOT unsigned char
    char string[RESULT_BUFFER_SIZE+1] = {-1};   

    for (int i = 0; i < A_NUM_ROWS*B_NUM_COLS; i++) {
        // If external libraries not allowed, could just do mathematical operations on elements of trans_res_matrix
        // char a = '278'
        // string[0] == '2', string[1] == '7', string[2] == '8'
        // char b = '78'
        // string[0] == '7', string[1] == '8' 
        // char c = '8'
        // string[0] == '8'
        sprintf(string, "%d", trans_res_matrix[i]);

        // Common to all 1/2/3/4 digit numbers
        while (XUartPs_IsTransmitFull(uart_base_address));
        XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[0]);

        // Common to all 2/3/4 digit numbers
        if (trans_res_matrix[i] >= 10) {
            while (XUartPs_IsTransmitFull(uart_base_address));
            XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[1]);
        }

        // Common to all 3/4 digit numbers
        if (trans_res_matrix[i] >= 100) {
            while (XUartPs_IsTransmitFull(uart_base_address));
            XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[2]);
        }

        // Solely for 4 digit numbers
        if (trans_res_matrix[i] >= 1000) {
            while (XUartPs_IsTransmitFull(uart_base_address));
            XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[3]);
        }

        // Newline for next element
        while (XUartPs_IsTransmitFull(uart_base_address));
        XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, NEW_LINE);
    }
}


char concat_char_buffer(char* buffer_ptr, int tail_index) {
    // Compress each element of char_buffer into a singular char
    // eg. |2||5||5| ---> 255
    //     [0][1][2]

    u8 sum = 0;

    // Loop through all elements in the array
    // Note that we traverse the array TAIL->HEAD
    for (int i = 0; i <= tail_index; i++) {
        // Convert ASCII numbers into char numbers
        u8 buffer_element = buffer_ptr[tail_index-i] - '0';

        // Convert using basic ones, tens, hundreds formula
        u8 place_value = find_place(i);
        buffer_element = buffer_element * place_value;
        sum += buffer_element;
    }

    return sum;
}


u8 find_place(u8 loop_iteration) {
    return (loop_iteration == ONES) ? (u8) 1
         : (loop_iteration == TENS) ? (u8) 10
         : (u8) 100;
}