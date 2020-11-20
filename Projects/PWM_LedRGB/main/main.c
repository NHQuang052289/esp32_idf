#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "MQTT Debug";
volatile uint16_t red_mqtt = 0, green_mqtt = 0, blue_mqtt = 0;

void Set_Value(uint16_t red, uint16_t green, uint16_t blue);
void Set_Color(void);

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    char * recv_data;
    uint16_t my_len_data;

    int msg_id;
    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            msg_id = esp_mqtt_client_subscribe(client, "qn052289@gmail.com/RGB_Red", 2);
            ESP_LOGI(TAG, "Subscribe RED!, msg_id = %d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "qn052289@gmail.com/RGB_Green", 2);
            ESP_LOGI(TAG, "Subscribe GREEN!, msg_id = %d", msg_id);
            msg_id = esp_mqtt_client_subscribe(client, "qn052289@gmail.com/RGB_Blue", 2);
            ESP_LOGI(TAG, "Subscribe BLUE!, msg_id = %d", msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "\r\nMQTT_EVENT_DATA");

            my_len_data = event->data_len; 
            recv_data = malloc(my_len_data * sizeof(char));

            //test memory of event->data_len and malloc
            ESP_LOGI(TAG,"event->data_len = %d , malloc =  %d \r\n", event->data_len, my_len_data * sizeof(char));

            if (recv_data == NULL) {
                ESP_LOGI(TAG,"Error malloc ");
            }

            strncpy(recv_data, event->data, my_len_data);
            
            //test length of recv_data and event->data_len
            ESP_LOGI(TAG,"recv_data = %s --- lenOf(recv_data) = %d  --- event->data_len = %d \r\n", recv_data, strlen(recv_data), my_len_data);

            if(event->topic[23] == 'R')
            {
                ESP_LOGI(TAG, "DATA: ------------>   %s", recv_data);
                red_mqtt = atoi(recv_data);
                ESP_LOGI(TAG, "RED_Value = %d", red_mqtt);
            }
            else if(event->topic[23] == 'G')
            {   
                ESP_LOGI(TAG, "DATA: ------------>  %s", recv_data);
                green_mqtt = atoi(recv_data);
                ESP_LOGI(TAG, "GREEN_Value = %d", green_mqtt);
            }
            else if(event->topic[23] == 'B')
            {
                ESP_LOGI(TAG, "DATA ------------> : %s", recv_data);
                blue_mqtt = atoi(recv_data);
                ESP_LOGI(TAG, "BLUE_Value = %d", blue_mqtt);
            }
            Set_Value(red_mqtt, green_mqtt, blue_mqtt); 

            free(recv_data);
            if (recv_data != NULL) recv_data = NULL;
            break;
        
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;

    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://www.maqiatto.com:1883",
        .username = "qn052289@gmail.com",
        .password = "182739"
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}


/*Config timer and channels*/
void PWM_Config(void)
{
    /*config timer*/
    ledc_timer_config_t myTimer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    /*init timer*/
    ledc_timer_config(&myTimer);

    /*Config Red channel*/
    ledc_channel_config_t Red_Channel = {
        .gpio_num = GPIO_NUM_14,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_FADE_END};
    
    /*init Red channel*/
    ledc_channel_config(&Red_Channel);

    /*Config Green channel*/
    ledc_channel_config_t Green_Channel = {
        .gpio_num = GPIO_NUM_12,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_FADE_END};
    
    /*init Green channel*/
    ledc_channel_config(&Green_Channel);

    /*Config Blue channel*/
    ledc_channel_config_t Blue_Channel = {
        .gpio_num = GPIO_NUM_13,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_FADE_END};
    
    /*init Blue channel*/
    ledc_channel_config(&Blue_Channel);
}

void Set_Value(uint16_t red, uint16_t green, uint16_t blue)
{
    ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, red, 0);
    ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, green, 0);
    ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, blue, 0);
}
void Set_Color(void)
{
    Set_Value(0, 0, 0);
    vTaskDelay(500/portTICK_PERIOD_MS);

    Set_Value(256, 0, 0);
    vTaskDelay(500/portTICK_PERIOD_MS);

    Set_Value(0, 256, 0);
    vTaskDelay(500/portTICK_PERIOD_MS);

    Set_Value(0, 0, 256);
    vTaskDelay(500/portTICK_PERIOD_MS);

    /*rainbow*/
    /*for(int color = 0; color <= 768; color++)
    {
        if(color <= 256)
        {
            Set_Value(256 - color, color, 0);
        }
        else if(color <= 512)
        {
            Set_Value(0, 256 - (color - 256), color - 256);
        }
        else
        {
            Set_Value(color - 513, 0, 256 - (color - 513));
        }

        vTaskDelay(10/portTICK_PERIOD_MS);
    }*/


}
void app_main()
{   
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();

    PWM_Config();
    ledc_fade_func_install(0);
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0, 300, LEDC_FADE_NO_WAIT);
    ledc_fade_func_install(1);
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, 0, 300, LEDC_FADE_NO_WAIT);
    ledc_fade_func_install(2);
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, 0, 300, LEDC_FADE_NO_WAIT);

    Set_Color();
    Set_Value(0, 0, 0);
    vTaskDelay(500/portTICK_PERIOD_MS);

    // while(1)
    // {
    //     vTaskDelay(100/portTICK_PERIOD_MS);
    //     Set_Value(red_mqtt, green_mqtt, blue_mqtt);   
    // }
    
}