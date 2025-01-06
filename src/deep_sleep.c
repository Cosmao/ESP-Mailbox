#include "deep_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "soc/soc_caps.h"
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static char *TAG = "DEEP_SLEEP";

#if SOC_RTC_FAST_MEM_SUPPORTED
static RTC_DATA_ATTR struct timeval sleep_enter_time;
#endif

void enable_timer_wake(unsigned long wakeup_time_sec) {
  ESP_LOGI(TAG, "Enabling wakeup timer, %lus", wakeup_time_sec);
  ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void enable_rtc_io_wake(int wakeup_pin, int level) {
  ESP_LOGI(TAG, "Enabling RTC_IO wakeup on pin %d", wakeup_pin);
  ESP_ERROR_CHECK(rtc_gpio_pullup_en(wakeup_pin));
  ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(wakeup_pin, level));
}

void start_deep_sleep(esp_mqtt_client_handle_t mqtt_client) {
  ESP_LOGI(TAG, "Disabling mqtt");
  struct timeval start, now;
  gettimeofday(&start, NULL);
  while (esp_mqtt_client_get_outbox_size(mqtt_client) > 0) {
    ESP_LOGI(TAG, "Message left to send, waiting");
    gettimeofday(&now, NULL);
    if (now.tv_sec - 1 > start.tv_sec) {
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  esp_mqtt_client_unsubscribe(mqtt_client, mqtt_topic);
  esp_mqtt_client_disconnect(mqtt_client);
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
  ESP_ERROR_CHECK(gpio_set_direction(wakeup_pin, GPIO_MODE_INPUT));
  while (gpio_get_level(wakeup_pin) == 1) {
    gettimeofday(&current_time, NULL);
    if (current_time.tv_sec - max_seconds_wait >= circuit_open_time.tv_sec) {
      return 1;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  return 0;
}

uint8_t enable_rtc_if_closed(gpio_num_t wakeup_pin) {
#define numMeasurements 5
  uint8_t numHigh = 0;
  for (uint8_t i = 0; i < numMeasurements; i++) {
    numHigh += wait_for_low(wakeup_pin, 0);
  }
  if (numHigh == 0) {
    enable_rtc_io_wake(wakeup_pin, 1);
    return 1;
  }
  return 0;
}

wake_actions handle_wake_source(gpio_num_t wakeup_pin) {
  switch (get_wake_source()) {
  case ESP_SLEEP_WAKEUP_UNDEFINED: {
    if (enable_rtc_if_closed(wakeup_pin)) {
      return WAKE_ACTION_REBOOT;
    } else {
      return WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
    }
  }
  case ESP_SLEEP_WAKEUP_TIMER: {
    if (enable_rtc_if_closed(wakeup_pin)) {
      return WAKE_ACTION_SEND_ALIVE;
    } else {
      return WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
    }
  }
  case ESP_SLEEP_WAKEUP_EXT0: {
    if (enable_rtc_if_closed(wakeup_pin)) {
      return WAKE_ACTION_SEND_DISTANCE;
    } else {
      return WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
    }
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
#define buffSize 100
  while (action != WAKE_ACTION_NO_ACTION) {
    char buff[buffSize];
    switch (action) {
    case WAKE_ACTION_NO_ACTION: {
      return;
    }
    case WAKE_ACTION_SEND_DISTANCE: {
      esp_err_t ret = ESP_OK;
      ret = wait_for_distance(distance_struct);

      if (ret == ESP_OK) {
        snprintf(
            buff, buffSize,
            "{\"distance\":{\"1\":%ld,\"2\":%ld,\"3\":%ld},\"lid\":\"closed\"}",
            distance_struct->measured_array[0],
            distance_struct->measured_array[1],
            distance_struct->measured_array[2]);
      } else {
        snprintf(buff, buffSize,
                 "{\"error\":\"Distance-error\",\"lid\":\"closed\"}");
      }
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      enable_rtc_if_closed(WAKEUP_PIN);
      break;
    }
    case WAKE_ACTION_SEND_ALIVE: {
      snprintf(buff, buffSize, "{\"status\":\"alive\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      action = WAKE_ACTION_NO_ACTION;
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
      snprintf(buff, buffSize, "{\"lid\":\"open\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_SEND_UNK_ERROR: {
      snprintf(buff, buffSize, "{\"error\":\"Unknown error\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      enable_rtc_if_closed(WAKEUP_PIN);
      break;
    }
    case WAKE_ACTION_REBOOT: {
      snprintf(buff, buffSize, "{\"reboot\":\"true\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      /*Hopping there since the powerbank seems fucked*/
      action = WAKE_ACTION_SEND_DISTANCE;
      break;
    }
    }
  }
}
