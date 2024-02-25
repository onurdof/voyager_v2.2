#ifndef __UART_INTERFACE
#define __UART_INTERFACE


#define TX0_PIN     43
#define RX0_PIN     44

#define UART_QUEUE_SIZE    10

void init_RS232();
void rx_task(void* pvParameters);


#endif



