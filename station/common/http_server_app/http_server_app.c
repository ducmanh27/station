#include "http_server_app.h"
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
// #include "protocol_examples_common.h"
#include <esp_http_server.h>
static httpd_req_t *REG;
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
static const char *TAG = "HTTP_SEVER";

static http_post_callback_t http_post_switch_callback = NULL;
static http_post_callback_t http_post_relay1_callback = NULL;
static http_post_callback_t http_post_relay2_callback = NULL;
static http_get_callback_t http_get_dht11_callback = NULL;
static http_post_callback_t http_post_slider_callback = NULL;
static http_post_callback_t http_post_wifi_info_callback = NULL;
static http_get_data_callback_t http_get_rgb_data_callback = NULL;

httpd_handle_t server = NULL;

static esp_err_t rgb_get_handler(httpd_req_t *req)
{
    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    int buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        char * buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char value[32];
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "color", value, sizeof(value)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => query1=%s", value);
            }
            http_get_rgb_data_callback(value, 6);
        }
        free(buf);
    }
    return ESP_OK;
}

static const httpd_uri_t get_rgb = {
    .uri = "/rgb",
    .method = HTTP_GET,
    .handler = rgb_get_handler,
    .user_ctx = NULL};

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html"); // Nhúng giao diện HTML vào
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static const httpd_uri_t get_dht11 = {
    .uri = "/dht11",
    .method = HTTP_GET,
    .handler = hello_get_handler,
    .user_ctx = NULL};

void dht11_response(char *data, int len)
{
    httpd_resp_send(REG, data, len);

}

static esp_err_t dht11_get_handler(httpd_req_t *req)
{
    REG = req;
    // const char *resp_str = (const char *)"{\"temperature\": \"27\",\"humidity\": \"80\"}";
    // httpd_resp_send(req, (const char*)resp_str, strlen(resp_str));
    http_get_dht11_callback();
    return ESP_OK;
}

static const httpd_uri_t get_data_dht11 = {
    .uri = "/getdatadht11",
    .method = HTTP_GET,
    .handler = dht11_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

static esp_err_t data_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    printf("DATA: %s\n", buf);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t post_data = {
    .uri = "/data",
    .method = HTTP_POST,
    .handler = data_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

static esp_err_t sw1_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    http_post_switch_callback((char *)buf, req->content_len);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t sw1_post_data = {
    .uri = "/switch1",
    .method = HTTP_POST,
    .handler = sw1_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};
static esp_err_t sw2_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    http_post_relay1_callback((char *)buf, req->content_len);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t sw2_post_data = {
    .uri = "/switch2",
    .method = HTTP_POST,
    .handler = sw2_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};
static esp_err_t sw3_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    http_post_relay2_callback((char *)buf, req->content_len);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t sw3_post_data = {
    .uri = "/switch3",
    .method = HTTP_POST,
    .handler = sw3_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};
static esp_err_t wifi_info_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    http_post_wifi_info_callback((char *)buf, req->content_len);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t wifi_info_post_data = {
    .uri = "/wifiinfo",
    .method = HTTP_POST,
    .handler = wifi_info_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};
static esp_err_t slider_post_handler(httpd_req_t *req)
{
    char buf[100];
    httpd_req_recv(req, buf, req->content_len);
    http_post_slider_callback((char *)buf, req->content_len);

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t slider_post_data = {
    .uri = "/slider",
    .method = HTTP_POST,
    .handler = slider_post_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx = NULL};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    if (strcmp("/dht11", req->uri) == 0)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "/dht11 URI is not available");
        /* Return ESP_OK to keep underlying socket open */
        return ESP_OK;
    }
    /* For any other URI send 404 and close socket */
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Some 404 error message");
    return ESP_FAIL;
}

void start_webserver(void)
{

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &get_data_dht11);
        httpd_register_uri_handler(server, &get_rgb);

        httpd_register_uri_handler(server, &get_dht11);
        httpd_register_uri_handler(server, &post_data);
        httpd_register_uri_handler(server, &sw1_post_data);
        httpd_register_uri_handler(server, &sw2_post_data);
        httpd_register_uri_handler(server, &sw3_post_data);
        httpd_register_uri_handler(server, &slider_post_data);
        httpd_register_uri_handler(server, &wifi_info_post_data);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    else
    {
        ESP_LOGI(TAG, "Error starting server!");
    }
}

void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}
void http_set_callback_switch(void *cb)
{
    http_post_switch_callback = cb;
}
void http_set_callback_relay1(void *cb)
{
    http_post_relay1_callback = cb;
}
void http_set_callback_relay2(void *cb)
{
    http_post_relay2_callback = cb;
}
void http_set_callback_dht11(void *cb)
{
    http_get_dht11_callback = cb;
}
void http_set_callback_slider(void *cb)
{
    http_post_slider_callback = cb;
}
void http_set_callback_wifi_info(void *cb)
{
    http_post_wifi_info_callback = cb;
}
void http_set_callback_rgb(void *cb)
{
    http_get_rgb_data_callback = cb;
}
