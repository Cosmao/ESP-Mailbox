#include "ultrasonic_distance.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include <rom/ets_sys.h>
#include <stdint.h>
#include <time.h>

static const char *TAG = "ULTRASONIC";

void start_measurement(gpio_num_t gpio_pin_trig) {
  gpio_set_level(gpio_pin_trig, 1);
  ets_delay_us(10);
  gpio_set_level(gpio_pin_trig, 0);
}

/* Busy wait for the echo response
 * First we wait for signal to go high
 * then we measure how long the response is and return*/
long wait_for_echo(gpio_num_t gpio_pin_echo, int16_t timeout, uint8_t level) {
  long micros_waited = 0;
  while (gpio_get_level(gpio_pin_echo) != level) {
    if (micros_waited++ > timeout) {
      return -1;
    }
    ets_delay_us(1);
  }
  micros_waited = 0;
  int64_t time_start = esp_timer_get_time();
  while (gpio_get_level(gpio_pin_echo) == level) {
    if (micros_waited++ > timeout) {
      return -1;
    }
    ets_delay_us(1);
  }
  int64_t time_end = esp_timer_get_time();
  int64_t time_elapsed = time_end - time_start;
  return (long)time_elapsed;
}

/* Speed of sound at sea level 0°C 331m/s
 * Its only a rough estimate this, should be good enough */
long get_distance(long u_seconds) {
  if (u_seconds <= 0) {
    return -1;
  }
  /*, m/s * microseconds * 10^3 = mm */
  return u_seconds * 340 / 2 / 1000;
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

  ESP_LOGI(TAG, "Trig: %d\nEcho: %d\nTimeout: %d\nLong is %du bytes",
           distance_struct->gpio_trigger, distance_struct->gpio_echo,
           distance_struct->timeout_in_u_seconds, sizeof(long));

  while (1) {
    long total_distance = 0;
    uint8_t total_measurements = 0;
    for (int16_t i = 0; i < numberOfMeasurements; i++) {
      start_measurement(distance_struct->gpio_trigger);
      long microsForEcho = wait_for_echo(
          distance_struct->gpio_echo, distance_struct->timeout_in_u_seconds, 1);
      if (microsForEcho == -1) {
        ESP_LOGE(TAG, "Error in distance read loop %d", i);
      } else {
        total_measurements++;
        long distance_in_mm = get_distance(microsForEcho);
        total_distance += distance_in_mm;
        distance_struct->measured_array[i] = distance_in_mm;
        ESP_LOGI(TAG, "Loop %d measured: %ldmm", i, distance_in_mm);
      }
      /* delaying to let the sensor reset */
      vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    if (total_measurements == 0) {
      ESP_LOGE(TAG, "0 measurements taken!");
    } else {
      distance_struct->average_measured = total_distance / total_measurements;
      ESP_LOGI(TAG, "Total distance: %ldmm", total_distance);
      ESP_LOGI(TAG, "Average distance: %ldmm",
               distance_struct->average_measured);
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
  distance_struct->task_done = 1;
  vTaskDelete(NULL);
}
