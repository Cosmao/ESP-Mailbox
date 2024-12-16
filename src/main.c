#include "deep_sleep.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "soc/gpio_num.h"

#define WAKEUP_PIN GPIO_NUM_7
static char *TAG = "MAIN";

void app_main(void) {
  get_wake_source();
  if (wait_for_low(WAKEUP_PIN, 30)) {
    ESP_LOGE(TAG, "Circuit open too long, disabling gpio wake");
    enable_timer_wake(20);
  } else {
    enable_rtc_io_wake(WAKEUP_PIN, 1);
    enable_timer_wake(20);
  }
  start_deep_sleep();
}
