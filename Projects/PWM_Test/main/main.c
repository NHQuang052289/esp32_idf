#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>

#define MY_WIFI_SSID CONFIG_EXAMPLE_WIFI_SSID
#define MY_WIFI_PASS CONFIG_EXAMPLE_WIFI_PASSWORD
#define PIN GPIO_NUM_2
#define TAG "Project"

/*....................Setting HTTP connect.................*/
esp_err_t Get_Handler(httpd_req_t *request)
{
    char *buffer;
    size_t buffer_length;

    buffer_length = httpd_req_get_hdr_value_len(request, "Host") + 1;
    if (buffer_length > 1)
    {
        buffer = malloc(buffer_length);

        if (httpd_req_get_hdr_value_str(request, "Host", buffer, buffer_length) == ESP_OK)
        {
            ESP_LOGI(TAG, "In Header => Host: %s", buffer);
        }
        free(buffer);
    }

    buffer_length = httpd_req_get_url_query_len(request) + 1;
    if (buffer_length > 1)
    {
        buffer = malloc(buffer_length);
        if (httpd_req_get_url_query_str(request, buffer, buffer_length) == ESP_OK)
        {
            ESP_LOGI(TAG, "URL query => %s", buffer);
            char param[32];
            if (httpd_query_key_value(buffer, "gpio", param, sizeof(param)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Received query parameter => gpio = %s", param);
            }
        }
    }
}

/*Init URI*/
httpd_uri_t myURI = {
    .uri = "/pwm",
    .method = HTTP_GET,
    .handler = Get_Handler,
    .user_ctx = NULL,
};

/*Function for starting the webserver*/
httpd_handle_t Start_WebServer
{
    /*Generate default configuration*/
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /*Empty handle to esp_http_server*/
    httpd_handle_t server = NULL;

    /*Start httpd server*/
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_prot);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &myURI);
        return server;
    }
    ESP_LOGI(TAG, "Error starting server!!!");
    return NULL;
}

static esp_err_t Event_Handler(void *context, system_event_t *event)
{
    httpd_handle_t *server = (httpd_handle_t *)context;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "SYSTEM EVENT STATION START");
        ESP_ERROR_CHECK(esp_wifi_connect());
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM EVENT STATION GOT IP");
        ESP_LOGI(TAG, "GOT IP: '%s'", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

        if (*server == NULL)
        {
            *server = Start_WebSever();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "SYSTEM EVENT STATION DISCONNECTED");
        ESP_ERROR_CHECK(esp_wifi_connect());

        if (*server)
        {
            httpd_stop(server);
            *server = NULL;
        }
        break;

    default:
        break;
    }
    return ESP_OK;
}

/*Initialize Wifi*/
static void Init_Wifi(void *arg)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(Event_Handler, arg));
    wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting wifi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA), &wifi_config);
    ESP_ERROR_CHECK(esp_wifi_start());
}

/*Configuration timer*/
void PWM_Config(void)
{
    /*config timer*/
    ledc_timer_config_t myTimer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK};
    /*init timer*/
    ledc_timer_config(&myTimer);

    /*config channel*/
    ledc_channel_config_t myChannel = {
        .gpio_num = 2,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .intr_type = LEDC_INTR_FADE_END};
    /*init channel*/
    ledc_channel_config(&myChannel);
}

void app_main()
{
    PWM_Config();

    ledc_fade_func_install(0);
    ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0, 1000, LEDC_FADE_WAIT_DONE);

    while (1)
    {
        //ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0, 1000, LEDC_FADE_WAIT_DONE);
        //ledc_set_fade_time_and_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 1024, 1000, LEDC_FADE_WAIT_DONE);
        for (int i = 0; i < 1024; i++)
        {
            ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, i, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);

        for (int i = 1024; i > 0; i--)
        {
            ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, i, 0);
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
