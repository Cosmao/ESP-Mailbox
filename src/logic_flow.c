#include "logic_flow.h"
#include "mqtt.h"

wake_actions handle_wake_source(gpio_num_t wakeup_pin) {
  switch (get_wake_source()) {
  case ESP_SLEEP_WAKEUP_UNDEFINED: {
    return WAKE_ACTION_REBOOT;
  }
  case ESP_SLEEP_WAKEUP_TIMER: {
    if (enable_rtc_wake_if_closed(wakeup_pin)) {
      return WAKE_ACTION_SEND_ALIVE;
    } else {
      return WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
    }
  }
  case ESP_SLEEP_WAKEUP_EXT0: {
    if (enable_rtc_wake_if_closed(wakeup_pin)) {
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
#define buffSize 32
#ifdef CONFIG_MQTT_QOS_0
#define MQTT_QOS 0
#elif CONFIG_MQTT_QOS_1
#define MQTT_QOS 1
#elif CONFIG_MQTT_QOS_2
#define MQTT_QOS 2
#endif
  while (action != WAKE_ACTION_NO_ACTION) {
    char buff[buffSize];
    switch (action) {
    case WAKE_ACTION_NO_ACTION: {
      return;
    }
    case WAKE_ACTION_SEND_CLOSED: {
      snprintf(buff, buffSize, "{\"lid\":\"closed\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, MQTT_QOS, 0,
                              true);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_SEND_ALIVE: {
      snprintf(buff, buffSize, "{\"status\":\"alive\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 2, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_WAIT_FOR_RTC_CLOSE: {
      if (wait_for_low(WAKEUP_PIN, RTC_TIMEOUT_SEC)) {
        action = WAKE_ACTION_SEND_ERROR_OPEN;
      } else {
        /*wake on lid open*/
        enable_rtc_io_wake(WAKEUP_PIN, 1);
        action = WAKE_ACTION_SEND_CLOSED;
      }
      break;
    }
    case WAKE_ACTION_SEND_ERROR_OPEN: {
      snprintf(buff, buffSize, "{\"lid\":\"open\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 2, 0, true);
      /*Wake on lid closed*/
      enable_rtc_io_wake(WAKEUP_PIN, 0);
      action = WAKE_ACTION_NO_ACTION;
      break;
    }
    case WAKE_ACTION_SEND_UNK_ERROR: {
      snprintf(buff, buffSize, "{\"error\":\"Unknown error\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 2, 0, true);
      action = WAKE_ACTION_NO_ACTION;
      enable_rtc_wake_if_closed(WAKEUP_PIN);
      break;
    }
    case WAKE_ACTION_REBOOT: {
      snprintf(buff, buffSize, "{\"reboot\":\"true\"}");
      esp_mqtt_client_enqueue(mqtt_client, mqtt_topic, buff, 0, 2, 0, true);
      action = WAKE_ACTION_WAIT_FOR_RTC_CLOSE;
      break;
    }
    }
  }
}
