#ifndef ultrasonic_distance_h
#define ultrasonic_distance_h

#include "soc/gpio_num.h"
#include <stdint.h>

int16_t wait_for_echo(gpio_num_t gpio_pin, uint8_t timeout, uint8_t level);
int16_t get_distance(int16_t u_seconds);
void measure_distance_task(void *pvParameters);

#endif // !ultrasonic_distance_h
