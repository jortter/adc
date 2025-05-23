#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event_base.h"
#include "mqtt_client.h"



#define MQTT_BROKER_URI         "mqtt://broker.emqx.io:1883"
#define MQTT_TOPIC_ESTADO       "UPV/PR2/2-08/sensor/estado"
#define MQTT_TOPIC_COMMANDS     "UPV/PR2/2-08/sensor/commands"
#define MQTT_TOPIC_TEMP         "UPV/PR2/2-08/sensor/temperatura"
#define MQTT_TOPIC_HUM          "UPV/PR2/2-08/sensor/humedad"
#define MQTT_TOPIC_CALIB        "UPV/PR2/2-08/sensor/calibracion"

static const char *MQTT_TAG = "MQTT_SENSOR";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT conectado!");
            esp_mqtt_client_subscribe(event->client, MQTT_TOPIC_COMMANDS, 1);
            esp_mqtt_client_publish(event->client, MQTT_TOPIC_ESTADO, "ESP32 conectado", 0, 1, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT desconectado");
            break;
        case MQTT_EVENT_DATA:
            if (strncmp(event->topic, MQTT_TOPIC_COMMANDS, event->topic_len) == 0 &&
                strncmp(event->data, "calibrar", event->data_len) == 0) {
                extern void calibrar_humedad(void);
                calibrar_humedad();
            }
            break;
        default:
            break;
    }
}

void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void mqtt_publicar_dato(const char *tipo, const char *valor) {
    const char *topic = strcmp(tipo, "humedad") == 0 ? MQTT_TOPIC_HUM : MQTT_TOPIC_TEMP;
    esp_mqtt_client_publish(mqtt_client, topic, valor, 0, 1, 0);
}

void mqtt_publicar_calibracion(const char *etapa, uint32_t valor) {
    char msg[64];
    snprintf(msg, sizeof(msg), "{\"%s\": %lu}", etapa, (unsigned long)valor);
    esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_CALIB, msg, 0, 1, 0);
}
