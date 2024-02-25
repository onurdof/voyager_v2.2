#include <stdio.h>
#include "pedal_encoder.h"
//freeRTOS part
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
// gpio part
#include "driver/gpio.h"
// ESP part
#include "esp_sleep.h"
#include "esp_log.h"

pcnt_unit_handle_t speed_pcnt_unit = NULL;
QueueHandle_t speed_queue;
SemaphoreHandle_t xSemaphore = NULL;

bool smooth_data = false;
bool start_speed = true;

static bool pedal_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_wakeup;
    QueueHandle_t queue = (QueueHandle_t)user_ctx;
    // send event data to queue, from this interrupt callback
    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
    return (high_task_wakeup == pdTRUE);
}

void init_PEDAL_ENC(void)
{
    speed_queue = xQueueCreate(10, sizeof(int));
    xSemaphore = xSemaphoreCreateBinary();

    printf("install pcnt unit \n");
    pcnt_unit_config_t unit_config = {
        .high_limit = SPEED_PCNT_HIGH_LIMIT,
        .low_limit = SPEED_PCNT_LOW_LIMIT,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &speed_pcnt_unit));

    printf("set glitch filter \n");
    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(speed_pcnt_unit, &filter_config));

    printf("install pcnt channels \n");
    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = PEDAL_ENC_EP2A,
        .level_gpio_num = -1,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(speed_pcnt_unit, &chan_a_config, &pcnt_chan_a));
    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = PEDAL_ENC_EP2B,
        .level_gpio_num = PEDAL_ENC_EP2A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(speed_pcnt_unit, &chan_b_config, &pcnt_chan_b));

    printf("set edge and level actions for pcnt channels \n");
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_HOLD));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_HOLD));

    printf("add watch points and register callbacks \n");
    int watch_points[] = {SPEED_PCNT_LOW_LIMIT, -50, 0, 50, SPEED_PCNT_HIGH_LIMIT};
    for (size_t i = 0; i < sizeof(watch_points) / sizeof(watch_points[0]); i++)
    {
        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(speed_pcnt_unit, watch_points[i]));
    }
    pcnt_event_callbacks_t cbs = {
        .on_reach = pedal_pcnt_on_reach,
    };

    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(speed_pcnt_unit, &cbs, speed_queue));

    printf("enable pcnt unit \n");
    ESP_ERROR_CHECK(pcnt_unit_enable(speed_pcnt_unit));
    printf("clear pcnt unit \n");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(speed_pcnt_unit));
    printf("start pcnt unit \n");
    ESP_ERROR_CHECK(pcnt_unit_start(speed_pcnt_unit));
}


void pedal_enc_task(void *args)
{
    int event_count = 0;
    pedal_speed_t *rot_speed_data = (pedal_speed_t *)args;
    
    //printf("Speed Encoder Task Started ... \n");
    while (1)
    {
        if (xQueueReceive(speed_queue, &event_count, pdMS_TO_TICKS(10)))
        {
            // ESP_LOGI(SPEED_TAG, "Watch point event, count: %d", event_count);
        }
        else
        {
            ESP_ERROR_CHECK(pcnt_unit_get_count(speed_pcnt_unit, &rot_speed_data->pulse_count));
            //printf("Pulse count: %d \n", rot_speed_data->pulse_count);
        }
    }
}


void pedal_speed_task(void *arg)
{
    pedal_speed_t *rot_speed_data = (pedal_speed_t *)arg;
    //printf("SPEED TASK STARTED ... \n");
    for (;;)
    {
        if (start_speed)
        {
            rot_speed_data->last_speed = rot_speed_data->curr_speed;
            vTaskDelay(100 / portTICK_PERIOD_MS);
            (rot_speed_data->curr_speed) = ((rot_speed_data->pulse_count) / 10);
            rot_speed_data->pulse_count = 0;
            pcnt_unit_clear_count(speed_pcnt_unit);
            vTaskDelay(10 / portTICK_PERIOD_MS);
            //printf("Current speed : %d Last speed : %d \n",rot_speed_data->curr_speed,rot_speed_data->last_speed);
            smooth_data = true;
        }
        else
        {
            (rot_speed_data->curr_speed) = 0;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void pedal_smooth_speed_task(void *arg)
{
    pedal_speed_t *speed_data = (pedal_speed_t *)arg;
    int abs_val = 0;
    printf("Smooth speed task started .. \n");
    for (;;)
    { 
            abs_val = abs((speed_data->curr_speed) - (speed_data->last_speed));
            //printf("abs val : %d , curr speed : %d , last speed : %d   \n ",abs_val,speed_data->curr_speed, speed_data->last_speed);
            if (abs_val < 1)
            {
                speed_data->smooth_speed = speed_data->last_speed;
                //printf("smooth speed  = %d  \n ", speed_data->smooth_speed);
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            else if (abs_val >= 1)
            {
                if ((speed_data->curr_speed) > (speed_data->last_speed))
                {
                    for (uint8_t i = speed_data->last_speed; i < (speed_data->curr_speed); i++)
                    {
                        speed_data->smooth_speed = i;
                        vTaskDelay(10 / portTICK_PERIOD_MS);
                        //printf("smooth speed  = %d  \n ", speed_data->smooth_speed);
                    }
                }
                else if ((speed_data->curr_speed) < (speed_data->last_speed))
                {
                    for (uint8_t i = speed_data->last_speed; i > (speed_data->curr_speed); i--)
                    {
                        speed_data->smooth_speed = i;
                        vTaskDelay(10 / portTICK_PERIOD_MS);
                        //printf("smooth speed  = %d  \n ", speed_data->smooth_speed);
                    }
                }
                else
                {
                    speed_data->smooth_speed = speed_data->smooth_speed;
                    vTaskDelay(10 / portTICK_PERIOD_MS);
                    //printf("smooth speed  = %d  \n ", speed_data->smooth_speed);
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);

            }
    }
}
