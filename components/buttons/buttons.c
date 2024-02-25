#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "stepper.h"

// ESP ERROR
#include "esp_system.h"
#include "esp_log.h"

#include "buttons.h"

// intercommunication package header
#include "intercommunication.h"

void init_BUTTONS(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_config_t output_conf = {};
    output_conf.intr_type = GPIO_INTR_DISABLE;
    output_conf.mode = GPIO_MODE_OUTPUT;
    output_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    output_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    output_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    gpio_config(&output_conf);
}

void voyager_interrupt_task(void* pvParameters)
{
    intercommunication_t_ptr intercomm_ptr  = (intercommunication_t_ptr)pvParameters;
    uint8_t* data = NULL;
    data = (uint8_t*)malloc(BUTTONS_QUEUE_SIZE * sizeof(uint8_t));
    for (;;) {
        data[GEAR_UP_IDX] = gpio_get_level(GEAR_UP_PIN);
        data[GEAR_DOWN_IDX] = gpio_get_level(GEAR_DOWN_PIN);
        data[PHYSICAL_BRAKE_IDX] = gpio_get_level(PHYSICAL_BRAKE_BUTTON_PIN);
        vTaskDelay(200/portTICK_PERIOD_MS);
        xQueueSend(intercomm_ptr->button_queue, data, portMAX_DELAY);
        
          
    }
    free(data);      
}

void voyager_gear_task(void* pvParameters)
{
    uint8_t received_data[3];
    intercommunication_t_ptr intercomm_ptr  = (intercommunication_t_ptr)pvParameters;
    intercomm_ptr->gear = 1;
    //uint32_t motor_pose = 0;
    
    while (1) 
    {
        
        if(xQueueReceive(intercomm_ptr->button_queue, received_data, portMAX_DELAY) == pdTRUE) 
        {
            if(received_data[GEAR_UP_IDX] == PRESSED)
            {
                if(intercomm_ptr->gear >= 5)
                {
                    intercomm_ptr->gear = 5;
                }
                else
                {
                    //motor_pose += 4000;
                    //printf("Motor pose : %ld \n",motor_pose);
                    //move_motor(intercomm_ptr,motor_pose);
                    intercomm_ptr->gear += 1;
                }  
            }
            else if(received_data[GEAR_DOWN_IDX] == PRESSED)
            {
                if(intercomm_ptr->gear <= 1)
                {
                    intercomm_ptr->gear = 1;
                }
                else
                {
                    //motor_pose -= 4000;
                    //printf("Motor pose : %ld \n",motor_pose);
                    //move_motor(intercomm_ptr,motor_pose);
                    intercomm_ptr->gear -= 1;
                    
                }
            }
            else if(received_data[PHYSICAL_BRAKE_IDX] == PRESSED)
            {
            /**
             * @todo : Fren verisi oyuna gönderilecek
             * -> Change gear task.
            */
            }
        }
        
        
        
    }
}
