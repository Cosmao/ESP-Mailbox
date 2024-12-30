#ifndef mqtt_h
#define mqtt_h

#include "mqtt_client.h"

esp_mqtt_client_handle_t mqtt_start(void);
void mqtt_task(void *pvParameter);

#endif // !mqtt_h
