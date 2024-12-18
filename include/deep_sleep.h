#ifndef deep_sleep_h
#define deep_sleep_h

#include "mqtt_client.h"
#include "soc/gpio_num.h"
#include <stdint.h>

void enable_timer_wake(int wakeup_time_sec);
void enable_rtc_io_wake(int GPIO_PORT, int level);
void start_deep_sleep(esp_mqtt_client_handle_t mqtt_client);
void get_wake_source(void);
uint8_t wait_for_low(gpio_num_t wakeup_pin, int max_seconds_wait);

#endif // !deep_sleep_h
