#include "deep_sleep.h"
#include "esp_err.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "ultrasonic_distance.h"

#define WAKEUP_PIN CONFIG_ESP_RTC_WAKEUP_PIN
#define WAKEUP_TIME_HOURS CONFIG_ESP_WAKEUP_TIME_IN_HOURS
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

  esp_mqtt_client_handle_t mqtt_client = mqtt_enable();
  wake_actions action = WAKE_ACTION_NO_ACTION;

  enable_timer_wake(WAKEUP_TIME_HOURS * 60 * 60);

  action = handle_wake_source(WAKEUP_PIN);
  handle_wake_actions(action, mqtt_client);

  start_deep_sleep(mqtt_client);
}
