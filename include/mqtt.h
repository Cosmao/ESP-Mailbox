#ifndef mqtt_h
#define mqtt_h

#include "mqtt_client.h"

const char *mqtt_topic = CONFIG_ESP_MQTT_TOPIC;
const char *mqtt_endpoint = CONFIG_ESP_MQTT_ENDPOINT;

esp_mqtt_client_handle_t mqtt_start(void);
esp_mqtt_client_handle_t mqtt_enable(void);

#endif // !mqtt_h
