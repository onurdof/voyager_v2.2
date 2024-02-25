#ifndef __INTERCOMM
#define __INTERCOMM

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"


#define HEADER_IDX      0
#define FAN_IDX         1
#define FORCEMENT_IDX   2




/**
 * @brief : Type should be one of the following
 * -TYPE_NORMAL     0
 * -TYPE_FORCEMENT  1
 * -TYPE_DOWNHILL   2
 * @note : NOT USED
*/
#define GET_GEAR_INDEX(gear_number, type) \
    (((gear_number - 1) * 3) + type)

/**
 * @note : NOT USED
*/
#define GET_TYPE(data)\
    (data-1)

typedef struct intercommunication
{
    QueueHandle_t rx_queue;
    QueueHandle_t button_queue;
    QueueHandle_t gear_queue;
    uint32_t motor_old_pose;
    uint32_t motor_current_pose;
    bool calibrated;
    uint8_t gear;

}intercommunication_t,*intercommunication_t_ptr;



#endif
