#include <stdio.h>
#include "feedback.h"
#include "intercommunication.h"
#include "stepper.h"
#include "driver/gpio.h"



static const uint32_t downhill_effects[5] = {0, 2000, 4000, 5000, 7000};
static const uint32_t forcement_effects[5] = {2000, 6000, 10000, 14000, 18000};
static const uint32_t normal_effects[5] = {0, 4000, 8000, 12000, 16000};

static uint8_t previous_fan_data = 0;
static uint8_t previous_forcement_state = 0;
static uint8_t previous_gear_state = 1; // initial gear is 1

static void add_wind_effect(uint8_t param)
{
    // Check if the current fan data is different from the previous one
    if (param != previous_fan_data)
    {
        switch (param)
        {
        case 0:
            gpio_set_level(FAN1_PIN, 1);
            gpio_set_level(FAN2_PIN, 1);
            break;
        case 1:
            gpio_set_level(FAN1_PIN, 0);
            break;
        case 2:
            gpio_set_level(FAN1_PIN, 0);
            gpio_set_level(FAN2_PIN, 0);
            break;
        default:
            break;
        }
        // Update the previous fan data
        previous_fan_data = param;
    }
}
static void add_forcement_effect(intercommunication_t_ptr intercomm_ptr, uint8_t param)
{ 
    if(param != previous_forcement_state || intercomm_ptr->gear != previous_gear_state)
    {
        switch (param)
        {
        case 1:
            move_motor(intercomm_ptr,normal_effects[(intercomm_ptr->gear - 1 )]);
            break; 
        case 2:
            move_motor(intercomm_ptr,forcement_effects[(intercomm_ptr->gear - 1 )]);
            break;
        case 3 :
            move_motor(intercomm_ptr,downhill_effects[(intercomm_ptr->gear - 1 )]);
            break;
        default:    
            break;
        }
        //printf("Gear : %d \n -- motor pose : %ld \n",intercomm_ptr->gear,intercomm_ptr->motor_current_pose);
        previous_forcement_state = param;
        previous_gear_state = intercomm_ptr->gear;
    }

}



void add_feedback(void* pvParameters)
{
    uint8_t received_data[3] = {};
    intercommunication_t_ptr intercomm_ptr = (intercommunication_t_ptr) pvParameters;
    while (true)
    {
        if(xQueueReceive(intercomm_ptr->rx_queue, received_data, portMAX_DELAY) == pdTRUE)
        {
            //printf("Received fand data : %d - forcement data : %d \n",received_data[FAN_IDX], received_data[FORCEMENT_IDX]);
            //printf("Received gear : %d \n",intercomm_ptr->gear);
            add_wind_effect(received_data[FAN_IDX]);
            add_forcement_effect(intercomm_ptr, received_data[FORCEMENT_IDX]);
        }
    }
    
     
}
