#include <stdio.h>
#include "uart_interface.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include "freertos/queue.h"

// Intercommunication package header
#include "intercommunication.h"



// Function to send data to the queue
//The static keyword makes it local to the file it's defined in.
/*
 When a function is declared as static, it restricts its visibility to the translation unit 
 (the source file and any included headers) in which it is defined. 
 It means that the function is only accessible within the same file in which it is declared.
*/
static void send_data_to_queue(QueueHandle_t queue,uint8_t* data, size_t length) {
    xQueueSend(queue, data, portMAX_DELAY); // Send data to the queue
}

void init_RS232()
{
    const int uart_buffer_size = 1024;
    QueueHandle_t uart_queue;

    // 1 - Setting Communication Parameters
    const uart_config_t uart_config = {             
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
        
    // Check for configuration success
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    
    // 2 - Setting Communication Pins
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, TX0_PIN, RX0_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 3 - Driver Installation
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));
}

void rx_task(void* pvParameters)
{
    const uart_port_t uart_num = UART_NUM_0;
    int length = 3;
    uint8_t* data = (uint8_t*)malloc(length * sizeof(uint8_t));
        if (data == NULL) {
            ESP_LOGE("RX_TASK", "Memory allocation failed!");
        }
    intercommunication_t_ptr intercomm_ptr = (intercommunication_t_ptr)pvParameters;

    while (1)
    {
        if (length > 0) {
            uart_read_bytes(uart_num, data, length, 100); // Read data string from the buffer
            send_data_to_queue(intercomm_ptr->rx_queue,data, length);
        }
    }
    free(data);
}
