#ifndef LEDC_APP_H
#define LEDC_APP_H

void ledc_init(void);
void ledc_add_pin(int pin, int channel);
void ledc_set(int channel, int duty);
#endif