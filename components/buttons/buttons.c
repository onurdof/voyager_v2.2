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

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void init_BUTTONS(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    gpio_config_t output_conf = {};
    output_conf.intr_type = GPIO_INTR_DISABLE;
    output_conf.mode = GPIO_MODE_OUTPUT;
    output_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    output_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    output_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    gpio_config(&output_conf);

    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GEAR_UP_PIN, gpio_isr_handler, (void *)GEAR_UP_PIN);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GEAR_DOWN_PIN, gpio_isr_handler, (void *)GEAR_DOWN_PIN);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(PHYSICAL_BRAKE_BUTTON_PIN, gpio_isr_handler, (void *)PHYSICAL_BRAKE_BUTTON_PIN);
}

void voyager_interrupt_task(void *pvParameters)
{
    intercommunication_t_ptr intercomm_ptr = (intercommunication_t_ptr)pvParameters;
    intercomm_ptr->gear = 1;
    // uint8_t *data = NULL;
    // data = (uint8_t *)malloc(BUTTONS_QUEUE_SIZE * sizeof(uint8_t));
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            if (gpio_get_level(io_num) == 0)
            {
                //printf("Interrupt occured .. %ld pin \n", io_num);
                switch (io_num)
                {
                case GEAR_UP_PIN:
                    if (intercomm_ptr->gear >= 5)
                    {
                        intercomm_ptr->gear = 5;
                    }
                    else
                    {
                        intercomm_ptr->gear += 1;
                        intercomm_ptr->usb_gear = (1ULL << 1);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                    }
                    break;

                case GEAR_DOWN_PIN:
                    // data[GEAR_DOWN_IDX] = gpio_get_level(GEAR_DOWN_PIN);
                    if (intercomm_ptr->gear <= 1)
                    {
                        intercomm_ptr->gear = 1;
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                    }
                    else
                    {
                        intercomm_ptr->gear -= 1;
                        intercomm_ptr->usb_gear = (1ULL << 2);
                        vTaskDelay(200 / portTICK_PERIOD_MS);
                    }
                    break;

                case PHYSICAL_BRAKE_BUTTON_PIN:
                    // data[PHYSICAL_BRAKE_IDX] = gpio_get_level(PHYSICAL_BRAKE_BUTTON_PIN);
                    intercomm_ptr->usb_gear = (1ULL << 0);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    break;

                default:
                    intercomm_ptr->usb_gear = 0;
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                    break;
                }
            }
            else
            {

                intercomm_ptr->usb_gear = 0;
            }
        }
        else
        {
            intercomm_ptr->usb_gear = 0;
        }

        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

