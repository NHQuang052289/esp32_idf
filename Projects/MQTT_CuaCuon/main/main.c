
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#define PIN GPIO_NUM_2

#define PIN_UP      GPIO_NUM_32
#define PIN_DOWN    GPIO_NUM_33

static const char *TAG = "MQTT_EXAMPLE";
u8_t status_up = 0, status_down = 0;


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{   
    // esp_mqtt_event_handle_t eventPause;
    // esp_mqtt_client_handle_t clientPause = eventPause->client;

    esp_mqtt_client_handle_t client = event->client;

    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            /* After MQTT connection is done, we will test topics with pub/sub. 
             * Do not forget to change topic data after make menuconfig. */
            
            msg_id = esp_mqtt_client_subscribe(client, "qn052289@gmail.com/ControlLed", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);            
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC = %.*s\r\n", event->topic_len, event->topic);
            printf("DATA = %.*s\r\n", event->data_len, event->data);
            if(event->data[0] == 'U') 
            {
                msg_id = esp_mqtt_client_publish(client, "qn052289@gmail.com/Status","UP", 3, 2, 0);
                ESP_LOGI(TAG, "Sent UP to Status Topic, msg_id = %d", msg_id);
                status_down = 0;
                status_up = 1;
                ESP_LOGI(TAG, "UP");
                
            }
            else if(event->data[0] == 'D') 
            {
                msg_id = esp_mqtt_client_publish(client, "qn052289@gmail.com/Status","DOWN", 5, 2, 0);
                ESP_LOGI(TAG, "Sent DOWN to Status Topic, msg_id = %d", msg_id);
                status_up = 0;
                status_down = 1;
                ESP_LOGI(TAG, "DOWN");
            }
            else if(event->data[0] == 'P') {  
                msg_id = esp_mqtt_client_publish(client, "qn052289@gmail.com/Status","PAUSE", 6, 2, 0);
                ESP_LOGI(TAG, "Sent PAUSE to Status Topic, msg_id = %d", msg_id); 
                status_up = 0;
                status_down = 0;
                ESP_LOGI(TAG, "PAUSE");
            }
            gpio_set_level(PIN_UP, status_up);
            gpio_set_level(PIN_DOWN, status_down);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
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

void app_main()
{
    gpio_pad_select_gpio(PIN);
    gpio_set_direction(PIN, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_UP);
    gpio_set_direction(PIN_UP, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_DOWN);
    gpio_set_direction(PIN_DOWN, GPIO_MODE_OUTPUT);

    gpio_set_level(PIN_UP, 0);
    gpio_set_level(PIN_DOWN, 0);

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
}
