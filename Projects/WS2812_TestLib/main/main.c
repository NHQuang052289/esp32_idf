#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "protocol_examples_common.h"
#include "esp_log.h"

#include "WS2812.h"

void app_main()
{
    hornbillLights_begin((rmt_channel_t) 0, (gpio_num_t)16, (uint16_t)60);
    hornbillLights_fullShow(52, 235, 180);

}