#include "deep_sleep.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "ultrasonic_distance.h"
#include <stdlib.h>
#include <time.h>

#define WAKEUP_PIN CONFIG_ESP_RTC_WAKEUP_PIN
#define WAKEUP_TIME_HOURS CONFIG_ESP_WAKEUP_TIME_IN_HOURS
#define WAKEUP_TIME_SEC (WAKEUP_TIME_HOURS * 60 * 60)
#define RTC_TIMEOUT_SEC CONFIG_ESP_RTC_TIMEOUT_SEC
#define ECHO_PIN CONFIG_ESP_ECHO_PIN
#define TRIG_PIN CONFIG_ESP_TRIG_PIN
#define DISTANCE_TIMEOUT CONFIG_ESP_DISTANCE_TIMEOUT_U_SEC

static char *TAG = "MAIN";

void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  distance_measurements *distance_struct =
      (distance_measurements *)malloc(sizeof(distance_measurements));
  if (distance_struct == NULL) {
    ESP_LOGE(TAG, "Failed to malloc distance struct");
    esp_restart();
  }
  distance_struct->task_done = 0;
  distance_struct->gpio_echo = ECHO_PIN;
  distance_struct->gpio_trigger = TRIG_PIN;
  distance_struct->timeout_in_u_seconds = DISTANCE_TIMEOUT;

  esp_mqtt_client_handle_t mqtt_client = mqtt_enable();
  wake_actions action = WAKE_ACTION_NO_ACTION;

  enable_timer_wake(WAKEUP_TIME_SEC);

  if (mqtt_client != NULL) {
    action = handle_wake_source(WAKEUP_PIN);
    handle_wake_actions(action, mqtt_client, distance_struct);
  }

  start_deep_sleep(mqtt_client);
}
