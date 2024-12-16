#include "deep_sleep.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "soc/gpio_num.h"
#include <stdlib.h>

#define WAKEUP_PIN GPIO_NUM_7
static char *TAG = "MAIN";

void app_main(void) {
  get_wake_source();
  int *pin = (int *)malloc(sizeof(int));
  if (pin == NULL) {
    ESP_LOGE(TAG, "Could not malloc an int?????");
    esp_restart();
  }
  *pin = GPIO_NUM_7;
  if (wait_for_low(WAKEUP_PIN)) {
    ESP_LOGE(TAG, "Circuit open too long, disabling gpio wake");
    enable_timer_wake(20);
    start_deep_sleep();
  } else {
    enable_rtc_io_wake(WAKEUP_PIN, 1);
    enable_timer_wake(20);
    start_deep_sleep();
  }
}
