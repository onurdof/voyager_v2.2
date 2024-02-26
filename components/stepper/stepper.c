
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "stepper_motor_encoder.h"
#include "stepper.h"


/**
 * TODO: ENABLE LIMIT_SWITCH HERE 
 * NOT IN THE BUTTONS INTERFACE Component
*/

static const char *TAG = "stepper";
const static uint32_t uniform_speed_hz = 20000; //15000 default

rmt_encoder_handle_t uniform_motor_encoder = NULL;
rmt_channel_handle_t motor_chan = NULL;
rmt_transmit_config_t tx_config = {
        .loop_count = 0,
};

void init_STEPPER(void)
{

    ESP_LOGI(TAG,"Limit switch configuration.");

    gpio_set_direction(LIMIT_SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(LIMIT_SWITCH_PIN,GPIO_PULLUP_ONLY);

    ESP_LOGI(TAG, "Initialize EN + DIR GPIO");
    gpio_config_t en_dir_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << STEP_MOTOR_GPIO_DIR | 1ULL << STEP_MOTOR_GPIO_EN,
    };
    ESP_ERROR_CHECK(gpio_config(&en_dir_gpio_config));

    ESP_LOGI(TAG, "Create RMT TX channel");
    
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select clock source
        .gpio_num = STEP_MOTOR_GPIO_STEP,
        .mem_block_symbols = 64,
        .resolution_hz = STEP_MOTOR_RESOLUTION_HZ,
        .trans_queue_depth = 10, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor_chan));

    ESP_LOGI(TAG, "Set spin direction");
    gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
    ESP_LOGI(TAG, "Enable step motor");
    gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);

    ESP_LOGI(TAG, "Create motor encoders");
    stepper_motor_curve_encoder_config_t accel_encoder_config = {
        .resolution = STEP_MOTOR_RESOLUTION_HZ,
        .sample_points = 500,
        .start_freq_hz = 500,
        .end_freq_hz = 1500,
    };
    rmt_encoder_handle_t accel_motor_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&accel_encoder_config, &accel_motor_encoder));

    stepper_motor_uniform_encoder_config_t uniform_encoder_config = {
        .resolution = STEP_MOTOR_RESOLUTION_HZ,
    };
 
    ESP_ERROR_CHECK(rmt_new_stepper_motor_uniform_encoder(&uniform_encoder_config, &uniform_motor_encoder));

    stepper_motor_curve_encoder_config_t decel_encoder_config = {
        .resolution = STEP_MOTOR_RESOLUTION_HZ,
        .sample_points = 500,
        .start_freq_hz = 1500,
        .end_freq_hz = 500,
    };
    rmt_encoder_handle_t decel_motor_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_stepper_motor_curve_encoder(&decel_encoder_config, &decel_motor_encoder));

    ESP_LOGI(TAG, "Enable RMT channel");
    ESP_ERROR_CHECK(rmt_enable(motor_chan));     
}



void limit_task(void* args)
{
    
    // uniform phase
    intercommunication_t_ptr intercomm_ptr = (intercommunication_t_ptr) args;
    ESP_LOGI(TAG, "Enable step motor");
    gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);
    tx_config.loop_count = 50000;
    ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_motor_encoder, &uniform_speed_hz, sizeof(uniform_speed_hz), &tx_config));
    for (;;)
    {
        if(!gpio_get_level(LIMIT_SWITCH_PIN)) // ENABLE LIMIT SWITCH FIRST
        {
    
            printf("Switch Reached .. \n");
            //ESP_LOGI(TAG, "Calibrating... \n");
            rmt_disable(motor_chan);
            

            vTaskDelay(1700/portTICK_PERIOD_MS); // Before 1500
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
            vTaskDelay(100/portTICK_PERIOD_MS);
            ESP_ERROR_CHECK(rmt_enable(motor_chan));
            tx_config.loop_count = 11000;  // 11 000 -> righ bicyle 10 000 -> left bicycle
            ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_motor_encoder, &uniform_speed_hz, sizeof(uniform_speed_hz), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
            vTaskDelay(50/portTICK_PERIOD_MS);
            intercomm_ptr->calibrated = true;
            //ESP_LOGI(TAG,"Step motor calibrated .. \n");
            vTaskDelete(NULL);
        }
        //ESP_LOGI(TAG,"Step motor not calibrated .. \n");
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}


void move_motor(intercommunication_t_ptr intercommptr,uint32_t pos)
{
    int diff = 0;    

    if(intercommptr->calibrated)
    {
        // move motor

        
        intercommptr->motor_current_pose = pos;

        diff = (intercommptr->motor_current_pose) - (intercommptr->motor_old_pose);
        //printf("Diff : %d \n..",diff);
        //ESP_ERROR_CHECK(rmt_enable(motor_chan));

        if(diff > 0)
        {
            //printf("Motor is moving forward .. \n");
            //gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);
            vTaskDelay(10/portTICK_PERIOD_MS);
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
            vTaskDelay(10/portTICK_PERIOD_MS);
            tx_config.loop_count = abs(diff);
            ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_motor_encoder, &uniform_speed_hz, sizeof(uniform_speed_hz), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
            //printf("Reached pose : %d . \n",motor_data->current_pose);
            intercommptr->motor_old_pose = intercommptr->motor_current_pose;
        }
        else if (diff == 0)
        {
            /**
             * @todo : Return an err code : DIFF IS 0
            */
        }
        else
        {
            vTaskDelay(10/portTICK_PERIOD_MS);
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
            vTaskDelay(10/portTICK_PERIOD_MS);
            tx_config.loop_count = abs(diff);
            ESP_ERROR_CHECK(rmt_transmit(motor_chan, uniform_motor_encoder, &uniform_speed_hz, sizeof(uniform_speed_hz), &tx_config));
            ESP_ERROR_CHECK(rmt_tx_wait_all_done(motor_chan, -1));
            intercommptr->motor_old_pose = intercommptr->motor_current_pose;
        }

    }
    else
    {
        /*
            @todo : İmplement an error checking mechanism and return 
            err code, MOTOR NOT CALIBRATED..
        */
    }
    vTaskDelay(10/portTICK_PERIOD_MS);

}



