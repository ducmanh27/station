/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "output_iot.h"
#include <http_server_app.h>
#include "dht11.h"
#include "ledc_app.h"
#include "ws2812b.h"
#include "protocol_examples_common.h"
/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define GPIO_DHT11 GPIO_NUM_4
#define GPIO_LED_BLINK GPIO_NUM_2
#define GPIO_RELAY1 GPIO_NUM_18
#define GPIO_RELAY2 GPIO_NUM_19
#define EXAMPLE_ESP_WIFI_SSID "P501 2.2G"
#define EXAMPLE_ESP_WIFI_PASS "123456789"
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY
#define DHT11_GPIO GPIO_NUM_4

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static struct dht11_reading dht11_last_data, dht11_cur_data;
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}
void switch_data_callback(char *data, uint16_t len)
{
    if (*data == '1')
    {
        printf("TEST1\n");
        output_io_createput_io_set_level(GPIO_NUM_2, 1);
    }
    else if (*data == '0')
    {
        printf("TEST0\n");
        output_io_createput_io_set_level(GPIO_NUM_2, 0);
    }
}
void relay1_data_callback(char *data, uint16_t len)
{
    if (*data == '1')
    {
        printf("RELAY 1 ON\n");
        output_io_createput_io_set_level(GPIO_RELAY1, 1);
    }
    else if (*data == '0')
    {
        printf("RELAY 1 OFF\n");
        output_io_createput_io_set_level(GPIO_RELAY1, 0);
    }
}
void relay2_data_callback(char *data, uint16_t len)
{
    if (*data == '1')
    {
        printf("RELAY 2 ON\n");
        output_io_createput_io_set_level(GPIO_RELAY2, 1);
    }
    else if (*data == '0')
    {
        printf("RELAY 2 OFF\n");
        output_io_createput_io_set_level(GPIO_RELAY2, 0);
    }
}
void wifi_info_data_callback(char *data, uint16_t len)
{
    char ssid[30] = "";
    char password[30] = "";
    char *token = strtok(data, "@");
    if (token)
        strcpy(ssid, token);
    token = strtok(NULL, "@");
    if (token)
        strcpy(password, token);
    printf("ssid: %s password: %s\n", ssid, password);
    // stop_webserver();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    esp_wifi_disconnect();
    esp_wifi_stop();

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    wifi_config_t wifi_config_get;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config_get);
    printf("--> ssid:   %s\n", wifi_config_get.sta.ssid);
    printf("--> password:   %s\n", wifi_config_get.sta.password);
    esp_wifi_start();
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 wifi_config_get.sta.ssid, wifi_config_get.sta.password);
    }
    // start_webserver();
}

void dht11_data_callback(void)
{
    char resp[100];
    sprintf(resp, "{\"temperature\": \"%d\",\"humidity\": \"%d\"}", dht11_last_data.temperature, dht11_last_data.humidity);
    dht11_response(resp, strlen(resp));
}
void slider_data_callback(char *data, uint16_t len)
{
    char number_str[10];
    memcpy(number_str, data, len + 1);

    int duty = atoi(number_str);
    printf("%d\n", duty);
    ledc_set(0, duty);
}
void rgb_data_callback(char *data, int len)
{
    printf("RGB: %s\n", data);
}

char recv_buf[512];

static void read_data_DHT11(void *pvParameters)
{
    for (;;)
    {
        dht11_cur_data = DHT11_read();
        if (dht11_cur_data.status == 0)
        {
            dht11_last_data = dht11_cur_data;
        }
        printf("T: %d H: %d\n", dht11_last_data.temperature, dht11_last_data.humidity);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    DHT11_init(GPIO_DHT11);
    ledc_init();
    ledc_add_pin(GPIO_NUM_2, 0);
    // ws2812b_Init(GPIO_NUM_15, 8);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    http_set_callback_switch(switch_data_callback);
    http_set_callback_dht11(dht11_data_callback);
    http_set_callback_slider(slider_data_callback);
    http_set_callback_relay1(relay1_data_callback);
    http_set_callback_relay2(relay2_data_callback);
    http_set_callback_wifi_info(wifi_info_data_callback);
    http_set_callback_rgb(rgb_data_callback);
    // output_io_create(GPIO_LED_BLINK);
    output_io_create(GPIO_RELAY1);
    output_io_create(GPIO_RELAY2);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    start_webserver();
    xTaskCreate(&read_data_DHT11, "read_data_DHT11", 4096, NULL, 5, NULL);
    // while (1)
    // {
    //     dht11_cur_data = DHT11_read();
    //     if (dht11_cur_data.status == 0)
    //     {
    //         dht11_last_data = dht11_cur_data;
    //     }
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}
