#include "deep_sleep.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include "ultrasonic_distance.h"
#include "wifi.h"
#include <stdlib.h>
#include <time.h>

// TODO: give info about wifi/mqtt to main thread
// Pass along the wakeup pin and sleep timer?
// all the good logic

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

  TaskHandle_t distance_handle;
  BaseType_t task_ret = xTaskCreate(measure_distance_task, "Distance-Task",
                                    3048, distance_struct, 5, &distance_handle);

  if (task_ret == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
    vTaskDelete(distance_handle);
    ESP_LOGE(TAG, "Failed to create task, deleting");
  }

  esp_mqtt_client_handle_t mqtt_client;
  TaskHandle_t mqtt_task_handle;

  task_ret = xTaskCreate(&mqtt_task, "MQTT_Task", 4096, mqtt_client, 3,
                         &mqtt_task_handle);
  if (task_ret == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) {
    vTaskDelete(mqtt_task_handle);
    ESP_LOGE(TAG, "Could not allocate memory for mqtt");
    esp_restart();
  }

  switch (get_wake_source()) {
  case ESP_SLEEP_WAKEUP_TIMER: {
    break;
  }
  case ESP_SLEEP_WAKEUP_EXT0: {
    break;
  }
  case ESP_SLEEP_WAKEUP_UNDEFINED: {
    break;
  }
  default: {
    break;
  }
  }

  if (wait_for_low(WAKEUP_PIN, 30)) {
    ESP_LOGE(TAG, "Circuit open too long, disabling gpio wake");
    enable_timer_wake(20);
  } else {
    enable_rtc_io_wake(WAKEUP_PIN, 1);
    enable_timer_wake(20);
  }

  while (distance_struct->task_done != 1)
    ;

  start_deep_sleep(mqtt_client);
}
