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
#include "wifi.h"
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

static char *TAG = "DEEP_SLEEP";

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
  struct timeval start, now;
  gettimeofday(&start, NULL);
  while (esp_mqtt_client_get_outbox_size(mqtt_client) > 0) {
    ESP_LOGI(TAG, "Message left to send, waiting");
    gettimeofday(&now, NULL);
    if (now.tv_sec - 10 > start.tv_sec) {
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  ESP_LOGI(TAG, "Disabling mqtt");
  esp_mqtt_client_unsubscribe(mqtt_client, mqtt_topic);
  esp_mqtt_client_disconnect(mqtt_client);
  esp_mqtt_client_stop(mqtt_client);
  ESP_LOGI(TAG, "Disabling wifi");
  dont_reconnect = 1;
  esp_wifi_disconnect();
  esp_wifi_stop();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Entering deep sleep");
  esp_deep_sleep_start();
}

esp_sleep_wakeup_cause_t get_wake_source() {
  switch (esp_sleep_get_wakeup_cause()) {
  case ESP_SLEEP_WAKEUP_TIMER: {
    ESP_LOGI(TAG, "Woke up from Timer.");
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

/*Returns the state of the pin, instant if low with a max timeout for seconds on
 * a high state to prevent locking*/
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
    return WAKE_ACTION_REBOOT;
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
      return WAKE_ACTION_SEND_CLOSED;
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
                         esp_mqtt_client_handle_t mqtt_client) {
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
    case WAKE_ACTION_SEND_CLOSED: {
      snprintf(buff, buffSize, "{\"lid\":\"closed\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_SEND_ALIVE: {
      snprintf(buff, buffSize, "{\"status\":\"alive\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 1, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_WAIT_FOR_RTC_CLOSE: {
      if (enable_rtc_if_closed(WAKEUP_PIN)) {
        action = WAKE_ACTION_SEND_CLOSED;
      } else {
        action = WAKE_ACTION_SEND_ERROR_OPEN;
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
      action = WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
      break;
    }
    }
  }
}
