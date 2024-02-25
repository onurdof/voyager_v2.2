#ifndef __GIDON_ENCODER
#define __GIDON_ENCODER

#include "stdbool.h"
#include "driver/pulse_cnt.h"

#define EXAMPLE_PCNT_HIGH_LIMIT 360
#define EXAMPLE_PCNT_LOW_LIMIT  -360
#define GIDON_ENC_EP1A 47  // PCB C15 - C16
#define GIDON_ENC_EP1B 21

typedef int8_t gidon_enc_data;



void init_GIDON_ENC(void);
/**
 * @todo : Declare it static in the corresponding .c file

*/
bool gidon_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx);


void gidon_enc_task(void* args);

#endif