#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "reedSwitch.h"
#include <freertos/FreeRTOS.h>

void app_main() {
  ESP_LOGI("TEST", "HUGE msg");
  TaskHandle_t reedHandle;
  BaseType_t ret =
      xTaskCreate(&reedTask, "ReedSwitch", 2048, NULL, 3, &reedHandle);
  if (reedHandle != pdPASS) {
    ESP_LOGE("TASK", "Error creating reedSwitch task");
  }
  return;
}
