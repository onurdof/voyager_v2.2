/*
 * Voyager application layer 
 * All other layers in the component folder
 * 
 * Gidon encoder limits changed. Check for calibration and test for calibration
 * 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ESP ERRORS
#include "esp_system.h"
#include "esp_log.h"

// COMPONENTS
#include "uart_interface.h"
#include "stepper.h"
#include "buttons.h"
#include "intercommunication.h"
#include "feedback.h"
#include "gidon_encoder.h"
#include "pedal_encoder.h"



intercommunication_t intercomm = {};
void app_main(void)
{
    intercommunication_t_ptr intercomm_ptr = &intercomm;

    intercomm_ptr->button_queue = xQueueCreate(BUTTONS_QUEUE_SIZE, sizeof(uint8_t*));
    if(intercomm_ptr->button_queue == NULL)
    {
        ESP_LOGE("APP_MAIN", "Failed to create button queue! \n");
    }

    intercomm_ptr->rx_queue = xQueueCreate(UART_QUEUE_SIZE, sizeof(uint8_t*));
    if(intercomm_ptr->rx_queue == NULL)
    {
        ESP_LOGE("APP_MAIN", "Failed to create uart queue! \n");
    }

    intercomm_ptr->gear_queue = xQueueCreate(GEAR_QUEUE_SIZE, sizeof(uint8_t*));
    if(intercomm_ptr->gear_queue == NULL)
    {
        ESP_LOGE("APP_MAIN", "Failed to create gear queue! \n");
    }

    
    init_RS232();
    init_STEPPER();
    init_BUTTONS();
    init_PEDAL_ENC();
    init_GIDON_ENC();

    xTaskCreate(limit_task,"limit_task",4096,(void *)intercomm_ptr,4,NULL);
    xTaskCreate(rx_task, "uart_rx_task", 4096, (void *)intercomm_ptr, 4, NULL);
    xTaskCreate(voyager_interrupt_task, "voyager_interrupt_task", 4096, (void*)intercomm_ptr, 5, NULL);
    xTaskCreate(voyager_gear_task, "voyager_gear_task", 4096, (void*)intercomm_ptr, 5, NULL);
    xTaskCreate(add_feedback, "add_feedback", 4096, (void*)intercomm_ptr, 5, NULL);
    
}
