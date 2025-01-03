#include "include/mqtt.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "wifi.h"

extern const uint8_t client_cert_start[] asm("_binary_esp32_crt_start");
extern const uint8_t client_cert_end[] asm("_binary_esp32_crt_end");
extern const uint8_t client_key_start[] asm("_binary_esp32_key_start");
extern const uint8_t client_key_end[] asm("_binary_esp32_key_end");
extern const uint8_t server_cert_start[] asm("_binary_ca_crt_start");
extern const uint8_t server_cert_end[] asm("_binary_ca_crt_end");

static const char *TAG = "Mqtt status";
const char *mqtt_topic = CONFIG_ESP_MQTT_TOPIC;
const char *mqtt_endpoint = CONFIG_ESP_MQTT_ENDPOINT;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32,
           base, event_id);
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;
  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    msg_id = esp_mqtt_client_subscribe(client, mqtt_topic, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;

  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x",
               event->error_handle->esp_tls_last_esp_err);
      ESP_LOGI(TAG, "Last tls stack error number: 0x%x",
               event->error_handle->esp_tls_stack_err);
      ESP_LOGI(TAG, "Last captured errno : %d (%s)",
               event->error_handle->esp_transport_sock_errno,
               strerror(event->error_handle->esp_transport_sock_errno));
    } else if (event->error_handle->error_type ==
               MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
      ESP_LOGI(TAG, "Connection refused error: 0x%x",
               event->error_handle->connect_return_code);
    } else {
      ESP_LOGW(TAG, "Unknown error type: 0x%x",
               event->error_handle->error_type);
    }
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

esp_mqtt_client_handle_t mqtt_start(void) {
#define buffSize 100

  const esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.uri = mqtt_endpoint,
      .broker.verification.certificate = (const char *)server_cert_start,
      .broker.verification.skip_cert_common_name_check = true,
      .credentials = {
          .authentication =
              {
                  .certificate = (const char *)client_cert_start,
                  .key = (const char *)client_key_start,
              },
      }};

  ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes",
           esp_get_free_heap_size());
  ESP_LOGI(TAG, "Connecting to endpoint: %s", mqtt_endpoint);
  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  /* The last argument may be used to pass data to the event handler, in this
   * example mqtt_event_handler */
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler,
                                 NULL);
  ESP_ERROR_CHECK(esp_mqtt_client_start(client));
  return client;
}

esp_mqtt_client_handle_t mqtt_enable(void) {
  if (wifi_init_station()) {
    return mqtt_start();
  } else {
    ESP_LOGE(TAG, "WIFI DIDNT START");
    esp_restart();
  }
}
