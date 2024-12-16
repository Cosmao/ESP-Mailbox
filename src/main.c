#include "deep_sleep.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "soc/gpio_num.h"
#include <stdlib.h>

#define WAKEUP_PIN GPIO_NUM_7
static char *TAG = "MAIN";

void app_main(void) {
  int *pin = (int *)malloc(sizeof(int));
  if (pin == NULL) {
    ESP_LOGE(TAG, "Could not malloc an int?????");
    esp_restart();
  }
  *pin = GPIO_NUM_7;
  enable_timer_wake(20);
  enable_rtc_io_wake(WAKEUP_PIN, 1);
  xTaskCreate(deep_sleep_task, "deep_sleep_task", 4096, pin, 6, NULL);
}
