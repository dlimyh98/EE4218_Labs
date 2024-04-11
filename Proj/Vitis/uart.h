#ifndef COMMON_HEADER
    #define COMMON_HEADER
    #include "common.h"
#endif

#include "xuartps.h"

#define UART_DEVICE_ID  XPAR_XUARTPS_0_DEVICE_ID
#define UART_BASEADDR   XPAR_XUARTPS_0_BASEADDR
#define MY_BAUD_RATE    115200  // Realterm must match this
#define NEW_LINE        0xA
#define COMMA           0x2C
#define CONCAT_BUFFER_SIZE 3
#define RESULT_BUFFER_SIZE 4
#define ONES        0
#define TENS        1
#define HUNDREDS    2

int init_UART(XUartPs* Uart_Ps);
void override_uart_configs(XUartPs* Uart_Ps);

void receive_from_realterm(u32 uart_base_addr, char* recv_a_matrix, char* recv_b_matrix, char* recv_c_matrix, int* HARD_input_memory);
void send_to_realterm(u32 uart_base_address, int* trans_res_matrix);

char concat_char_buffer(char* buffer_ptr, int tail_index);
u8 find_place(u8 loop_iteration);