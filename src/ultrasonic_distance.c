#include "ultrasonic_distance.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "hal/gpio_types.h"
#include <rom/ets_sys.h>

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
