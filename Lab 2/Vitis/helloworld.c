// Register Summary - https://docs.xilinx.com/r/en-US/ug585-zynq-7000-SoC-TRM/Register-Summary?tocId=YpLmh7sf1iyVzENdSo7oGw
#include "xparameters.h"
#include "xuartps.h"
#include "xil_printf.h"
#include "stdio.h"

// All map to UART_PS_1 (the UART we are using)
#define UART_DEVICE_ID  XPAR_XUARTPS_0_DEVICE_ID
#define UART_BASEADDR   XPAR_XUARTPS_0_BASEADDR

#define MY_BAUD_RATE    115200  // Realterm must match this

#define NEW_LINE        0xA
#define COMMA           0x2C
#define CONCAT_BUFFER_SIZE 3
#define ONES        0
#define TENS        1
#define HUNDREDS    2

#define A_NUM_ROWS 2
#define A_NUM_COLS 2
#define B_NUM_ROWS 2
#define B_NUM_COLS 1
/*********************** FUNCTION DECLARATIONS **********************************/
int init_UART(XUartPs* Uart_Ps);
void override_uart_configs(XUartPs* Uart_Ps);

void receive_from_realterm(u32 uart_base_addr, char* recv_a_matrix, char* recv_b_matrix);
void do_processing(char* recv_a_matrix, char* recv_b_matrix, char* trans_res_matrix);
void send_to_realterm(u32 uart_base_address, char* trans_res_matrix);

char concat_char_buffer(char* buffer_ptr, int tail_index);
u8 find_place(u8 loop_iteration);
/********************************************************************************/

int main()
{
    XUartPs Uart_Ps;    // Instance of UART Driver. Passed around by functions to refer to SPECIFIC driver instance

    int init_uart_status = init_UART(&Uart_Ps);
    if (init_uart_status == XST_FAILURE) {
        xil_printf("Failed to initialize UART device");
        return XST_FAILURE;
    }

    override_uart_configs(&Uart_Ps);

    char recv_a_matrix[A_NUM_ROWS*A_NUM_COLS] = {0};
    char recv_b_matrix[B_NUM_ROWS*B_NUM_COLS] = {0};
    char trans_res_matrix[A_NUM_ROWS*B_NUM_COLS] = {0};
    
    xil_printf("Ready to accept files from Realterm\n");
    receive_from_realterm(UART_BASEADDR, recv_a_matrix, recv_b_matrix);

    xil_printf("Do processing\n");
    do_processing(recv_a_matrix, recv_b_matrix, trans_res_matrix);

    xil_printf("Send to Realterm\n");
    send_to_realterm(UART_BASEADDR, trans_res_matrix);

    return 0;
}


/*********************************** FUNCTION DEFINITIONS *********************************************/
void send_to_realterm(u32 uart_base_address, char* trans_res_matrix) {
    // Additional +1 to accomodate null-termination of strings
    // Note string literals must be char, NOT unsigned char
    char string[CONCAT_BUFFER_SIZE+1] = {-1};   

    for (int i = 0; i < A_NUM_ROWS*B_NUM_COLS; i++) {
        // If external libraries not allowed, could just do mathematical operations on elements of trans_res_matrix
        sprintf(string, "%d", trans_res_matrix[i]);

		XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[0]);
		XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[1]);

        if (trans_res_matrix[i] >= 100) {
            XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, string[2]);
        }

        // We don't need ',' after printing the last element
        if (i != (A_NUM_ROWS*B_NUM_COLS)-1) {
            XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, COMMA);
        }
    }

    // Finish off with a newline
    XUartPs_WriteReg(uart_base_address, XUARTPS_FIFO_OFFSET, NEW_LINE);
}


void do_processing(char* recv_a_matrix, char* recv_b_matrix, char* trans_res_matrix) {
    for (int i = 0; i < A_NUM_ROWS; i++) {
        // Note we use int to prevent overflow
        // Elements of the input matrices being between 0 and 127 will guarantee that 
        // the result will not exceed the representation range possible with 8-bits. n*127*127/256, where n = 4 is less than 255.
        int sum = 0;

        for (int j = 0; j < A_NUM_COLS; j++) {
            sum += recv_a_matrix[(i*A_NUM_COLS)+j] * recv_b_matrix[j];
        }

        trans_res_matrix[i] = (sum >> 8);
    }

    // Sanity Check
    /*
    for (int i = 0; i < A_NUM_ROWS * A_NUM_COLS; i++) {
        xil_printf("%c\n", recv_a_matrix[i]);
    }
    
    for (int i = 0; i < B_NUM_ROWS * B_NUM_COLS; i++) {
        xil_printf("%c\n", recv_b_matrix[i]);
    }*/
}


// Data is sent through .csv files via Realterm
// .csv files MUST be in Unix format (i.e consider line break as 0xA). Can use Vim to set fileformat to Unix.
// Also note that the file MUST have EOL character. Can use Vim to check also.
void receive_from_realterm(u32 uart_base_addr, char* recv_a_matrix, char* recv_b_matrix) {
    int valid_recv_count = 0;    // Number of elements in our usual Matrix

    char buffer[CONCAT_BUFFER_SIZE] = {0};  // Maximal integer value is '255', formed by 3 chars
    int num_insertions = 0;

    while(1) {
        // Check if all valid data has been received
        // Note that we need to check BEFORE we begin RX polling
        if (valid_recv_count == (A_NUM_ROWS*A_NUM_COLS)
                              + (B_NUM_ROWS*B_NUM_COLS)) return;

        // Polling until any data is received
        while (!XUartPs_IsReceiveData(uart_base_addr));

        // Read from Transmit and Receive FIFO[7:0]
        u8 recv_char = XUartPs_ReadReg(uart_base_addr, XUARTPS_FIFO_OFFSET);

        //xil_printf("Received %c\n", recv_char);

        // Parse the data accordingly
        if (recv_char == NEW_LINE || recv_char == COMMA) {
            // Newline, means we are going to 'next row' of matrix
            // Comma, means we are transitioning to next matrix 'element'
            u8 concat_char = concat_char_buffer(buffer, num_insertions-1);
            
            //xil_printf("concatChar %c\n", concat_char);

            if (valid_recv_count < A_NUM_ROWS*A_NUM_COLS) {
                *recv_a_matrix = concat_char;
                recv_a_matrix++;
            } 
            else if (A_NUM_ROWS*A_NUM_COLS <= valid_recv_count 
                    && valid_recv_count < A_NUM_ROWS*A_NUM_COLS + B_NUM_ROWS*B_NUM_COLS) {
                *recv_b_matrix = concat_char;
                recv_b_matrix++;
            }

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


// Compress each element of char_buffer into a singular char
// eg. |2||5||5| ---> 255
//     [0][1][2]
char concat_char_buffer(char* buffer_ptr, int tail_index) {
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


void override_uart_configs(XUartPs* Uart_Ps_ptr) {
	XUartPs_SetBaudRate(Uart_Ps_ptr, MY_BAUD_RATE);
}


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