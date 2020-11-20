#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h" //data logging
#include "nvs_flash.h" //permnant
#include "esp_bt.h"

//hornbill lights files
#include "ble.h"
#include "WS2812.h"

void app_main()
{
  

   

    hornbillLights_begin((rmt_channel_t) 0, (gpio_num_t)16, (uint16_t)10);
    hornbillLights_fullShow(235, 52, 213);

    


}   
