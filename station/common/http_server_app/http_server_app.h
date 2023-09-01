#ifndef __HTTP_SERVER_APP_H
#define __HTTP_SERVER_APP_H
#include "esp_err.h"
#include "hal/gpio_types.h"
#include <stdint.h>

typedef void (*http_post_callback_t)(char *data, uint16_t lenght);
typedef void (*http_get_callback_t)(void);
typedef void (*http_get_data_callback_t)(char *data, int len);

void dht11_response(char *data, int len);
void start_webserver(void);
void stop_webserver(void);
void http_set_callback_dht11(void *cb);
void http_set_callback_switch(void *cb);
void http_set_callback_slider(void *cb);
void http_set_callback_relay1(void *cb);
void http_set_callback_relay2(void *cb);
void http_set_callback_wifi_info(void *cb);
void http_set_callback_rgb(void *cb);
#endif