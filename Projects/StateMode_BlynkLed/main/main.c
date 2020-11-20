#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include "driver\gpio.h"

#define MY_WIFI_SSID CONFIG_EXAMPLE_WIFI_SSID
#define MY_WIFI_PASS CONFIG_EXAMPLE_WIFI_PASSWORD
#define PIN GPIO_NUM_2
#define TAG "Project"

u8_t status = 1;
esp_err_t Get_Handler(httpd_req_t *request)
{
  char* buffer;
  size_t buffer_lengh;

  buffer_lengh = httpd_req_get_hdr_value_len(request, "Host") + 1;
  if (buffer_lengh > 1)
  {
    buffer = malloc(buffer_lengh);

    if (httpd_req_get_hdr_value_str(request, "Host", buffer, buffer_lengh) == ESP_OK)
    {
      ESP_LOGI(TAG, "In header => Host: %s", buffer);
    }
    free(buffer);

    buffer_lengh = httpd_req_get_url_query_len(request) + 1;
    if (buffer_lengh > 1)
    {
      buffer = malloc(buffer_lengh);
      if (httpd_req_get_url_query_str(request, buffer, buffer_lengh) == ESP_OK)
      {
        ESP_LOGI(TAG, "URL query => %s", buffer);
        char param[32];
        if (httpd_query_key_value(buffer, "gpio", param, sizeof(param)) == ESP_OK)
        {
          ESP_LOGI(TAG, "Received query parameter => gpio = %s", param);
          status = !status;
          ESP_LOGI(TAG, "Status for the pin %d is %d", PIN, status);
          gpio_set_level(PIN, status);
        }
      }
      free(buffer);
    }
  }
  
  char* responses_str = "<br/><br/><h2><div align='center'>LED is <b>OFF</b></div></h2>";
  if(status)
  {
    responses_str = "<br/><br/><h2><div align='center'>LED is <b>ON</b></div></h2>";
  }
  httpd_resp_send(request, responses_str, strlen(responses_str));

  if(httpd_req_get_hdr_value_len(request, "Host") == 0)
  {
    ESP_LOGI(TAG, "Request headers lost");
  }

  return ESP_OK;
}

httpd_uri_t myURI = {
    .uri = "/control",
    .method = HTTP_GET,
    .handler = Get_Handler,
    .user_ctx = NULL
};

/*Function for starting the webserver*/
httpd_handle_t Start_WebSever(void)
{
  /* Generate default configuration */
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  /* Empty handle to esp_http_server */
  httpd_handle_t server = NULL;
  //Start httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if(httpd_start(&server, &config) == ESP_OK)
  {
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &myURI);
    return server;
  }
  ESP_LOGI(TAG, "Error starting server!!!");
  return NULL;
}

static esp_err_t Event_Handler(void* context, system_event_t *event)
{
  httpd_handle_t *server = (httpd_handle_t *) context;

  switch (event->event_id)
  {
    case SYSTEM_EVENT_STA_START:
      ESP_LOGI(TAG, "SYSTEM EVENT STATION START");
      ESP_ERROR_CHECK(esp_wifi_connect());
      break;
    
    case SYSTEM_EVENT_STA_GOT_IP:
      ESP_LOGI(TAG, "SYSTEM EVENT STATION GOT IP");
      ESP_LOGI(TAG, "GOT IP: '%s'", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

      if(*server == NULL)
      {
        *server = Start_WebSever();
      }
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "SYSTEM EVENT STATION DISCONNECTED");
      ESP_ERROR_CHECK(esp_wifi_connect());

      if(*server)
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

static void initialize_wifi(void *arg)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(Event_Handler, arg));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = MY_WIFI_SSID,
            .password = MY_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(nvs_flash_init());
    gpio_pad_select_gpio(PIN);
    gpio_set_direction(PIN, GPIO_MODE_OUTPUT);
    initialize_wifi(&server);
}  

