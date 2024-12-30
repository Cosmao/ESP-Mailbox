#ifndef deep_sleep_h
#define deep_sleep_h

#include "esp_sleep.h"
#include "mqtt_client.h"
#include "soc/gpio_num.h"
#include "ultrasonic_distance.h"
#include <stdint.h>

typedef enum {
  WAKE_ACTION_NO_ACTION,
  WAKE_ACTION_SEND_DISTANCE,
  WAKE_ACTION_SEND_ALIVE,
  WAKE_ACTION_WAIT_FOR_RTC_CLOSE,
  WAKE_ACTION_SEND_ERROR_OPEN,
  WAKE_ACTION_SEND_UNK_ERROR,
} wake_actions;

void enable_timer_wake(int wakeup_time_sec);
void enable_rtc_io_wake(int GPIO_PORT, int level);
void start_deep_sleep(esp_mqtt_client_handle_t mqtt_client);
esp_sleep_wakeup_cause_t get_wake_source(void);
uint8_t wait_for_low(gpio_num_t wakeup_pin, int max_seconds_wait);
uint8_t enable_rtc_if_closed(gpio_num_t wakeup_pin);
wake_actions handle_wake_source(gpio_num_t wakeup_pin);
void handle_wake_actions(wake_actions action,
                         esp_mqtt_client_handle_t mqtt_client,
                         distance_measurements *distance_struct);

#endif // !deep_sleep_h
