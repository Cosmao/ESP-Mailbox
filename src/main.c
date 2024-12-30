#include "deep_sleep.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "ultrasonic_distance.h"
#include <stdlib.h>
#include <time.h>

// TODO: give info about wifi/mqtt to main thread
// Pass along the wakeup pin and sleep timer?
// all the good logic
// Put the wakeup time and similar in NVS for device shadows

#define WAKEUP_PIN GPIO_NUM_7
#define WAKEUP_TIME_SEC 20
#define RTC_TIMEOUT_SEC 20
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
  distance_struct->gpio_echo = GPIO_NUM_4;
  distance_struct->gpio_trigger = GPIO_NUM_5;
  distance_struct->timeout_in_u_seconds = 5000;

  esp_mqtt_client_handle_t mqtt_client;
  TaskHandle_t mqtt_task_handle;
  wake_actions action = WAKE_ACTION_NO_ACTION;

  BaseType_t task_ret = xTaskCreate(&mqtt_task, "MQTT_Task", 4096, mqtt_client,
                                    3, &mqtt_task_handle);
  if (task_ret == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
    vTaskDelete(mqtt_task_handle);
    ESP_LOGE(TAG, "Could not allocate memory for mqtt");
    esp_restart();
  }

  enable_timer_wake(WAKEUP_TIME_SEC);

  action = handle_wake_source(WAKEUP_PIN);
  handle_wake_actions(action, mqtt_client, distance_struct);

  start_deep_sleep(mqtt_client);
}
