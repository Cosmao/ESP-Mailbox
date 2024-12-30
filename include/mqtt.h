#ifndef mqtt_h
#define mqtt_h

#include "mqtt_client.h"

esp_mqtt_client_handle_t mqtt_start(void);
esp_mqtt_client_handle_t mqtt_enable(void);

#endif // !mqtt_h
