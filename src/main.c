#include "deep_sleep.h"
#include "esp_err.h"
#include "logic_flow.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#define WAKEUP_PIN CONFIG_ESP_RTC_WAKEUP_PIN
#define WAKEUP_TIME_HOURS CONFIG_ESP_WAKEUP_TIME_IN_HOURS

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
