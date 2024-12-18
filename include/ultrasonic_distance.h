#ifndef ultrasonic_distance_h
#define ultrasonic_distance_h

#include "esp_err.h"
#include "soc/gpio_num.h"
#include <stdint.h>

#define numberOfMeasurements 3

typedef struct {
  gpio_num_t gpio_trigger;
  gpio_num_t gpio_echo;
  int8_t timeout_in_u_seconds;
  int16_t measured_array[numberOfMeasurements];
  int16_t average_measured;
  uint8_t task_done;
} distance_measurements;

int16_t wait_for_echo(gpio_num_t gpio_pin, uint8_t timeout, uint8_t level);
int16_t get_distance(int16_t u_seconds);
esp_err_t init_distance_gpio(gpio_num_t gpio_trigger, gpio_num_t gpio_echo);
void measure_distance_task(void *pvParameters);

#endif // !ultrasonic_distance_h
