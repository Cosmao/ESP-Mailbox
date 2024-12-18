#include "deep_sleep.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "soc/gpio_num.h"
#include "wifi.h"

#define WAKEUP_PIN GPIO_NUM_7
static char *TAG = "MAIN";

void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  get_wake_source();

  wifi_init_station();
  esp_mqtt_client_handle_t mqtt_client = mqtt_start();

  if (wait_for_low(WAKEUP_PIN, 30)) {
    ESP_LOGE(TAG, "Circuit open too long, disabling gpio wake");
    enable_timer_wake(20);
  } else {
    enable_rtc_io_wake(WAKEUP_PIN, 1);
    enable_timer_wake(20);
  }
  start_deep_sleep(mqtt_client);
}
