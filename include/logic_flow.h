#ifndef logic_flow_h
#define logic_flow_h

#include "deep_sleep.h"

wake_actions handle_wake_source(gpio_num_t wakeup_pin);
void handle_wake_actions(wake_actions action,
                         esp_mqtt_client_handle_t mqtt_client);

#endif // !logic_flow_h
