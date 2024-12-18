#include "ultrasonic_distance.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "hal/gpio_types.h"
#include <rom/ets_sys.h>
#include <time.h>

static const char *TAG = "ULTRASONIC";

/* Busy wait for the echo response */
int16_t wait_for_echo(gpio_num_t gpio_pin, uint8_t timeout, uint8_t level) {
  int16_t micros_waited = 0;
  while (gpio_get_level(gpio_pin) == level) {
    if (micros_waited > timeout) {
      return -1;
    }
    ets_delay_us(1);
  }
  return micros_waited;
}

/* Speed of sound at sea level 0°C 331m/s
 * Its only a rough estimate this, should be good enough */
int16_t get_distance(int16_t u_seconds) {
  if (u_seconds <= 0) {
    return -1;
  }
  /*, m/s * microseconds * 10^3 = mm */
  return 331 * u_seconds * 1000;
}

esp_err_t init_distance_gpio(gpio_num_t gpio_trigger, gpio_num_t gpio_echo) {
  esp_err_t ret = gpio_set_direction(gpio_trigger, GPIO_MODE_OUTPUT);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = gpio_set_direction(gpio_echo, GPIO_MODE_INPUT);
  return ret;
}

void measure_distance_task(void *pvParameters) {
  distance_measurements *distance_struct =
      (distance_measurements *)pvParameters;

  ESP_ERROR_CHECK(init_distance_gpio(distance_struct->gpio_trigger,
                                     distance_struct->gpio_echo));

  uint16_t total_distance = 0;
  uint8_t total_measurements = 0;
  for (int16_t i = 0; i > numberOfMeasurements; i++) {
    distance_struct->measured_array[i] = wait_for_echo(
        distance_struct->gpio_echo, distance_struct->timeout_in_u_seconds, 0);
    if (distance_struct->measured_array[i] == -1) {
      ESP_LOGE(TAG, "Error in distance read loop %d", i);
    } else {
      total_measurements++;
      total_distance += distance_struct->measured_array[i];
    }
  }

  distance_struct->average_measured = total_distance / total_measurements;
  ESP_LOGE(TAG, "Average distance: %dmm", distance_struct->average_measured);
  distance_struct->task_done = 1;
  vTaskDelete(NULL);
}
