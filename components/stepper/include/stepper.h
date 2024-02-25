#ifndef __STEPPER
#define __STEPPER


#include <stdio.h>
#include "intercommunication.h"

///////////////////////////////Change the following configurations according to your board//////////////////////////////
#define LIMIT_SWITCH_PIN         5
#define STEP_MOTOR_GPIO_EN       11 
#define STEP_MOTOR_GPIO_DIR      12
#define STEP_MOTOR_GPIO_STEP     13
#define STEP_MOTOR_ENABLE_LEVEL  0 // TB6600 is enabled on low level
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0 // Switche doğru
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE 

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution
void init_STEPPER(void);
void limit_task(void* args);
void move_motor(intercommunication_t_ptr intercommptr,uint32_t pos);
void add_feedback(void* pvParameters);

#endif
