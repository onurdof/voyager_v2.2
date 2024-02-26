#include <stdio.h>
#include "gidon_encoder.h"
#include "driver/gpio.h"

//freeRTOS libs
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
// esp libs
#include "esp_log.h"
#include "esp_sleep.h"

QueueHandle_t queue = NULL;
pcnt_unit_handle_t pcnt_unit = NULL;

void init_GIDON_ENC(void)
{
    printf("install pcnt unit");
    pcnt_unit_config_t unit_config = {
        .high_limit = EXAMPLE_PCNT_HIGH_LIMIT,
        .low_limit = EXAMPLE_PCNT_LOW_LIMIT,
    };
    
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    printf("set glitch filter");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    printf("install pcnt channels");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = GIDON_ENC_EP1A,
        .level_gpio_num = GIDON_ENC_EP1B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = GIDON_ENC_EP1B,
        .level_gpio_num = GIDON_ENC_EP1A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    printf("add watch points and register callbacks");
    int watch_points[] = {EXAMPLE_PCNT_LOW_LIMIT, -50, 0, 50, EXAMPLE_PCNT_HIGH_LIMIT};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++) {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
    }
    pcnt_event_callbacks_t cbs = {
        .on_reach = gidon_pcnt_on_reach,
    };
    queue = xQueueCreate(10, sizeof(int));
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));

    printf("enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    printf("clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    printf("start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

bool gidon_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // send event data to queue, from this interrupt callback
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

void gidon_enc_task(void* args)
{
    
    int pulse_count = 0;
    int event_count = 0;
    gidon_enc_data * rotary_enc_ptr = (gidon_enc_data*)args;
    //printf("Rotary Encoder Task Started ... \n");
    while (1) {
        if (xQueueReceive(queue, &event_count, pdMS_TO_TICKS(10))) {
           //printf("Watch point event, count: %d\n", event_count);
        } 
        else {
            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
            //printf("Pulse count rotary: %d \n", pulse_count);
            (*rotary_enc_ptr) = (pulse_count / 4 ); // 2.65 for one 2.10 for the other
            //if((*rotary_enc_ptr) <= -127)
            //{
            //    (*rotary_enc_ptr) = -127;
            //}
            //else if((*rotary_enc_ptr)>= 127)
            //{
            //    (*rotary_enc_ptr) = 127;
            //}
            //printf("Gidon Encoder Value : %d \n",*(rotary_enc_ptr));
        }
        vTaskDelay(20/portTICK_PERIOD_MS);
    }
}