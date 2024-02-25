#ifndef __BUTTONS
#define __BUTTONS

#define BUTTONS_QUEUE_SIZE 3
#define GEAR_QUEUE_SIZE 1
#define PRESSED 0


#define GEAR_UP_IDX 0
#define GEAR_DOWN_IDX 1
#define PHYSICAL_BRAKE_IDX 2

#define PHYSICAL_BRAKE_BUTTON_PIN   4

#define GEAR_UP_PIN                 6
#define GEAR_DOWN_PIN               7

#define FAN1_PIN                    15 
#define FAN2_PIN                    16

#define GPIO_OUTPUT_PIN_SEL         ((1ULL<< FAN1_PIN) | (1ULL << FAN2_PIN))
#define GPIO_INPUT_PIN_SEL          ((1ULL<<GEAR_UP_PIN) | (1ULL<<GEAR_DOWN_PIN) | (1ULL<<PHYSICAL_BRAKE_BUTTON_PIN))

#define ESP_INTR_FLAG_DEFAULT 0
#define DEBOUNCE_DELAY_MS   500
void init_BUTTONS(void);
void voyager_interrupt_task(void* arg);
void voyager_gear_task(void* pvParameters);


#endif
