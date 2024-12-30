#include "deep_sleep.h"
#include "driver/rtc_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "hal/rtc_io_types.h"
#include "mqtt_client.h"
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

void start_deep_sleep(esp_mqtt_client_handle_t mqtt_client) {
  ESP_LOGI(TAG, "Disabling mqtt");
  esp_mqtt_client_stop(mqtt_client);
  ESP_LOGI(TAG, "Disabling wifi");
  esp_wifi_stop();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Entering deep sleep");
  gettimeofday(&sleep_enter_time, NULL);
  esp_deep_sleep_start();
}

esp_sleep_wakeup_cause_t get_wake_source() {
  struct timeval now;
  gettimeofday(&now, NULL);
  int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 +
                      (now.tv_usec - sleep_enter_time.tv_usec) / 1000;
  switch (esp_sleep_get_wakeup_cause()) {
  case ESP_SLEEP_WAKEUP_TIMER: {
    ESP_LOGI(TAG, "Woke up from Timer. Time spent in deep sleep: %dms",
             sleep_time_ms);
    return ESP_SLEEP_WAKEUP_TIMER;
  }
  case ESP_SLEEP_WAKEUP_EXT0: {
    ESP_LOGI(TAG, "Woke up from external RTC_IO");
    return ESP_SLEEP_WAKEUP_EXT0;
  }
  case ESP_SLEEP_WAKEUP_UNDEFINED:
  default:
    ESP_LOGI(TAG, "Not a deep sleep wake");
    return 0;
  }
}

uint8_t wait_for_low(gpio_num_t wakeup_pin, int max_seconds_wait) {
  struct timeval circuit_open_time, current_time;
  gettimeofday(&circuit_open_time, NULL);
  ESP_ERROR_CHECK(rtc_gpio_set_direction(wakeup_pin, RTC_GPIO_MODE_INPUT_ONLY));
  while (gpio_get_level(wakeup_pin) == 1) {
    ESP_LOGI(TAG, "Pin is %s",
             gpio_get_level(wakeup_pin) == 0 ? "low" : "high");
    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - max_seconds_wait >= circuit_open_time.tv_sec) {
      return 1;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return 0;
}

uint8_t enable_rtc_if_closed(gpio_num_t wakeup_pin) {
  if (!wait_for_low(wakeup_pin, 0)) {
    enable_rtc_io_wake(wakeup_pin, 1);
    return 1;
  }
  return 0;
}

wake_actions handle_wake_source(gpio_num_t wakeup_pin) {
  switch (get_wake_source()) {
  case ESP_SLEEP_WAKEUP_UNDEFINED:
  case ESP_SLEEP_WAKEUP_TIMER: {
    if (enable_rtc_if_closed(wakeup_pin)) {
      return WAKE_ACTION_SEND_ALIVE;
    } else {
      return WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
    }
  }
  case ESP_SLEEP_WAKEUP_EXT0: {
    return WAKE_ACTION_SEND_DISTANCE;
  }
  default: {
    return WAKE_ACTION_SEND_UNK_ERROR;
  }
  }
}

void handle_wake_actions(wake_actions action,
                         esp_mqtt_client_handle_t mqtt_client,
                         distance_measurements *distance_struct) {

#define WAKEUP_PIN CONFIG_ESP_RTC_WAKEUP_PIN
#define WAKEUP_TIME_SEC CONFIG_ESP_WAKEUP_TIME_IN_SEC
#define RTC_TIMEOUT_SEC CONFIG_ESP_RTC_TIMEOUT_SEC

  while (action != WAKE_ACTION_NO_ACTION) {
    switch (action) {
    case WAKE_ACTION_NO_ACTION: {
      break;
    }
    case WAKE_ACTION_SEND_DISTANCE: {
      esp_err_t ret = wait_for_distance(distance_struct);
      if (ret == ESP_OK) {
        esp_mqtt_client_publish(mqtt_client, "asd/asd", "DISTANCE HERE", 0, 1,
                                0);
      } else {
        esp_mqtt_client_publish(mqtt_client, "asd/asd", "DISTANCE THREAD ERROR",
                                0, 1, 0);
      }
      action = WAKE_ACTION_NO_ACTION;
      enable_rtc_io_wake(WAKEUP_PIN, 1);
      break;
    }
    case WAKE_ACTION_SEND_ALIVE: {
      // FIXME: Proper message and topic
      esp_mqtt_client_publish(mqtt_client, "asd/asd", "IM ALIVE", 0, 1, 0);
      action = WAKE_ACTION_NO_ACTION;
      enable_rtc_io_wake(WAKEUP_PIN, 1);
      break;
    }
    case WAKE_ACTION_WAIT_FOR_RTC_CLOSE: {
      if (wait_for_low(WAKEUP_PIN, RTC_TIMEOUT_SEC)) {
        action = WAKE_ACTION_SEND_ERROR_OPEN;
      } else {
        action = WAKE_ACTION_SEND_DISTANCE;
      }
      break;
    }
    case WAKE_ACTION_SEND_ERROR_OPEN: {
      esp_mqtt_client_publish(mqtt_client, "asd/asd", "ERROR IS OPEN", 0, 1, 0);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_SEND_UNK_ERROR: {
      esp_mqtt_client_publish(mqtt_client, "asd/asd", "VERY ERROR", 0, 1, 0);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    }
  }
}
