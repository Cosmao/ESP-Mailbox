#include "deep_sleep.h"
#include "driver/rtc_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/rtc_io_types.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static char *TAG = "DEEP_SLEEP";

#if SOC_RTC_FAST_MEM_SUPPORTED
static RTC_DATA_ATTR struct timeval sleep_enter_time;
#endif

void enable_timer_wake(int wakeup_time_sec) {
  ESP_LOGI(TAG, "Enabling wakeup timer, %ds", wakeup_time_sec);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void enable_rtc_io_wake(int wakeup_pin, int level) {
  ESP_ERROR_CHECK(rtc_gpio_pullup_en(wakeup_pin));
  ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(wakeup_pin, level));
  ESP_LOGI(TAG, "Enabling GPIO wakeup on pin %d", wakeup_pin);
}

void start_deep_sleep() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Entering deep sleep");
  gettimeofday(&sleep_enter_time, NULL);
  // enter deep sleep
  esp_deep_sleep_start();
}

void get_wake_source() {
  struct timeval now;
  gettimeofday(&now, NULL);
  int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 +
                      (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
  switch (esp_sleep_get_wakeup_cause()) {
  case ESP_SLEEP_WAKEUP_TIMER: {
    ESP_LOGI(TAG, "Woke up from Timer. Time spent in deep sleep: %dms",
             sleep_time_ms);
    break;
  }
  case ESP_SLEEP_WAKEUP_EXT0: {
    ESP_LOGI(TAG, "Woke up from external RTC_IO");
    break;
  }
  case ESP_SLEEP_WAKEUP_UNDEFINED:
  default:
    ESP_LOGI(TAG, "Not a deep sleep wake");
  }
}

uint8_t wait_for_low(gpio_num_t wakeup_pin) {
  struct timeval circuit_open_time, current_time;
  gettimeofday(&circuit_open_time, NULL);
  ESP_ERROR_CHECK(rtc_gpio_set_direction(wakeup_pin, RTC_GPIO_MODE_INPUT_ONLY));
  ESP_LOGI(TAG, "Pin is %s", gpio_get_level(wakeup_pin) == 0 ? "low" : "high");
  while (gpio_get_level(wakeup_pin) == 1) {
    ESP_LOGI(TAG, "Pin is %s",
             gpio_get_level(wakeup_pin) == 0 ? "low" : "high");
    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - 30 >= circuit_open_time.tv_sec) {
      return 1;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return 0;
}
