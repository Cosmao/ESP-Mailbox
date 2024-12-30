#ifndef ultrasonic_distance_h
#define ultrasonic_distance_h

#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "soc/gpio_num.h"
#include <stdint.h>

#define numberOfMeasurements 3

typedef struct {
  gpio_num_t gpio_trigger;
  gpio_num_t gpio_echo;
  int16_t timeout_in_u_seconds;
  long measured_array[numberOfMeasurements];
  long average_measured;
  uint8_t task_done;
} distance_measurements;

void start_measurement(gpio_num_t gpio_pin_trig);
long wait_for_echo(gpio_num_t gpio_pin_echo, int16_t timeout, uint8_t level);
long get_distance(long u_seconds);
esp_err_t init_distance_gpio(gpio_num_t gpio_trigger, gpio_num_t gpio_echo);
long get_measurement(distance_measurements *distance_struct);
void measure_distance_task(void *pvParameters);
esp_err_t wait_for_distance(distance_measurements *distance_struct,
                            TaskHandle_t task_handle);

#endif // !ultrasonic_distance_h
