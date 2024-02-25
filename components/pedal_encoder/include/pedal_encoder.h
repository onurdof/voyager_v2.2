#ifndef __PEDAL_ENCODER
#define __PEDAL_ENCODER
#include "driver/pulse_cnt.h"


#define SPEED_PCNT_HIGH_LIMIT 18000
#define SPEED_PCNT_LOW_LIMIT  -18000

// We use EP2A and EP2B cause it is mentioned like this in the pcb desing
#define PEDAL_ENC_EP2A 14 
#define PEDAL_ENC_EP2B 48 

typedef int8_t pedal_enc_speed_t;
typedef struct pedal_speed
{
    uint8_t curr_speed;
    uint8_t last_speed;
    int8_t smooth_speed;
    int pulse_count;
}pedal_speed_t;


void init_PEDAL_ENC(void);
void pedal_enc_task(void *args);
void pedal_speed_task(void *arg);
void pedal_smooth_speed_task(void *arg);

#endif