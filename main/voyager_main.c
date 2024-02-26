/*
 * Voyager application layer 
 * All other layers in the component folder
 * 
 * Gidon encoder limits changed. Check for calibration and test for calibration
 * 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// ESP ERRORS
#include "esp_system.h"
#include "esp_log.h"

// COMPONENTS
#include "uart_interface.h"
#include "stepper.h"
#include "buttons.h"
#include "intercommunication.h"
#include "feedback.h"
#include "gidon_encoder.h"
#include "pedal_encoder.h"

//USB HID

#include "tinyusb.h"
#include "class/hid/hid_device.h"




intercommunication_t intercomm = {};

// USB HID PART
#define TUSB_DESC_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)


/************* TinyUSB descriptors ****************/

/**
 * @brief HID report descriptor
 *

 */
const uint8_t hid_report_descriptor[] = {
    TUD_HID_REPORT_DESC_GAMEPAD()};

/**
 * @brief String descriptor
 */
const char *hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},    // 0: is supported language is English (0x0409)
    "DOF",                   // 1: Manufacturer
    "Voyager Device",        // 2: Product
    "1",                     // 3: Serials, should use chip ID2-S2	ESP32-S3
    "Voyager HID interface", // 4: HID
};

/**
 * @brief Configuration descriptor
 *
 * This is a simple configuration descriptor that defines 1 configuration and 1 HID interface
 */
static const uint8_t hid_configuration_descriptor[] = {
    // Configuration number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, 0, TUSB_DESC_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // Interface number, string index, boot protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(0, 4, false, sizeof(hid_report_descriptor), 0x81, 16, 10),
    // TUD_HID_INOUT_DESCRIPTOR(0,4,false,sizeof(hid_report_descriptor),0x81,0x80,16,10)
};

/********* TinyUSB HID callbacks ***************/

// Invoked when received GET HID REPORT DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
    // We use only one interface and one HID report descriptor, so we can ignore parameter 'instance'
    return hid_report_descriptor;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

/*
Device Descriptor developed by MOB
*/
const tusb_desc_device_t voyager_device_descriptor = {
    .bLength = sizeof(voyager_device_descriptor),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    /* Class type is not defined at the device descriptor level. The class type for a HID class device is defined by the Interface descriptor
        Subclass field is used to identify Boot devices
        The bDeviceClass and bDeviceSubClass fields in the Device Descriptor
        should not be used to identify a device as belonging to the HID class. Instead use
        the bInterfaceClass and bInterfaceSubClass fields in the Interface descriptor.
    */
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x505,
    .idProduct = 0x405,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03};

void init_USB(void)
{
    printf("USB initialization .. \n");
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = &voyager_device_descriptor,
        .string_descriptor = hid_string_descriptor,
        .string_descriptor_count = sizeof(hid_string_descriptor) / sizeof(hid_string_descriptor[0]),
        .external_phy = false,
        .configuration_descriptor = hid_configuration_descriptor,
    };

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
    printf("USB initialization DONE .. \n");
}



void app_send_hid(void *args)
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (tud_mounted())
        {
            tud_hid_gamepad_report(HID_ITF_PROTOCOL_NONE,intercomm.gidon_data, (int8_t)((intercomm.pedal_vars.smooth_speed) * 2), 0, 0, 0, 0, 0, intercomm.usb_gear);
        }
    }
}






void app_main(void)
{
    intercommunication_t_ptr intercomm_ptr = &intercomm;

    intercomm_ptr->button_queue = xQueueCreate(BUTTONS_QUEUE_SIZE, sizeof(uint8_t*));
    if(intercomm_ptr->button_queue == NULL)
    {
        ESP_LOGE("APP_MAIN", "Failed to create button queue! \n");
    }

    intercomm_ptr->rx_queue = xQueueCreate(UART_QUEUE_SIZE, sizeof(uint8_t*));
    if(intercomm_ptr->rx_queue == NULL)
    {
        ESP_LOGE("APP_MAIN", "Failed to create uart queue! \n");
    }

    //intercomm_ptr->gear_queue = xQueueCreate(GEAR_QUEUE_SIZE, sizeof(uint8_t*));
    //if(intercomm_ptr->gear_queue == NULL)
    //{
    //    ESP_LOGE("APP_MAIN", "Failed to create gear queue! \n");
    //}

    
    init_RS232();
    init_STEPPER();
    init_BUTTONS();
    init_PEDAL_ENC();
    init_GIDON_ENC();
    init_USB();

    xTaskCreate(limit_task,"limit_task",4096,(void *)intercomm_ptr,4,NULL);
    xTaskCreate(rx_task, "uart_rx_task", 4096, (void *)intercomm_ptr, 4, NULL);
    xTaskCreate(voyager_interrupt_task, "voyager_interrupt_task", 4096, (void*)intercomm_ptr, 5, NULL);
    //xTaskCreate(voyager_gear_task, "voyager_gear_task", 4096, (void*)intercomm_ptr, 5, NULL);
    xTaskCreate(add_feedback, "add_feedback", 4096, (void*)intercomm_ptr, 5, NULL);

    // Pedal tasks
    xTaskCreate(pedal_enc_task, "speed_enc_task", 4096,(void*)&intercomm_ptr->pedal_vars, 1, NULL);
    xTaskCreate(pedal_speed_task, "pedal_speed_task", 4096, (void*)&intercomm_ptr->pedal_vars, 1, NULL);
    xTaskCreate(pedal_smooth_speed_task, "pedal_smooth_speed_task", 4096, (void*)&intercomm_ptr->pedal_vars, 1, NULL);
    //Gidon task
    xTaskCreate(gidon_enc_task, "gidon_enc_task", 4096,(void*)&intercomm_ptr->gidon_data, 3, NULL);
    xTaskCreate(app_send_hid, "app_send_hid", 4096,NULL, 1, NULL);

    //while (true)
    //{
    //    printf("Pedal data : %d -- gidon data : %d -- \n",intercomm_ptr->pedal_vars.smooth_speed, intercomm_ptr->gidon_data);
    //    vTaskDelay(35/portTICK_PERIOD_MS);
    //}
    


    
}
