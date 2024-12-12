#include "reedSwitch.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "portmacro.h"
#include "soc/gpio_num.h"

#define pin GPIO_NUM_7

void reedTask(void *pvParam) {
  gpio_set_direction(pin, GPIO_MODE_INPUT);
  while (1) {
    uint8_t level = gpio_get_level(pin);
    ESP_LOGI("Reed", "Reed loop level %s", level == 1 ? "high" : "low");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
