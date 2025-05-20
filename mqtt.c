#include "mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG_MQTT = "MQTT";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "broker.hivemq.com",
        .port = 1883,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_publish_sensor_data(float humedad, float temperatura) {
    if (!client) return;
    char payload[100];
    int len = snprintf(payload, sizeof(payload),
                       "{\"humedad\":%.2f,\"temperatura\":%.2f}",
                       humedad, temperatura);
    esp_mqtt_client_publish(client,
                            "esp32/sensores",
                            payload,
                            len,
                            1,  /* QoS */
                            0   /* no retain */);
}
