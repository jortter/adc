// mqtt.c
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
#include "mqtt.h"


static const char *MQTT_TAG = "MQTT_SENSOR";    // Etiqueta para el registro de logs
static esp_mqtt_client_handle_t mqtt_client = NULL; // Cliente MQTT

// función para manejar eventos MQTT

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data; // Obtener el evento MQTT
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:  // Evento de conexión
            ESP_LOGI(MQTT_TAG, "MQTT conectado!");
            esp_mqtt_client_subscribe(event->client, MQTT_TOPIC_COMMANDS, 1);   // Suscribirse al tema de comandos
            esp_mqtt_client_publish(event->client, MQTT_TOPIC_ESTADO, "ESP32 conectado", 0, 1, 0);  // Publicar estado
            break;
        case MQTT_EVENT_DISCONNECTED:   // Evento de desconexión
            ESP_LOGI(MQTT_TAG, "MQTT desconectado");
            break;
        case MQTT_EVENT_DATA:   // Evento de recepción de datos
            if (strncmp(event->topic, MQTT_TOPIC_COMMANDS, event->topic_len) == 0 &&
                strncmp(event->data, "calibrar", event->data_len) == 0) {   // Comando de calibración
                extern void calibrar_humedad(void); // Declarar la función de calibración
                calibrar_humedad(); // Llamar a la función de calibración de humedad
            }
            break;
        default:
            break;
    }
}

// Función para iniciar el cliente MQTT
void mqtt_app_start(void) {
    // Configuración del cliente MQTT
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI
    };
    // Crear el cliente MQTT
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

// Función para publicar datos en el tema correspondiente
void mqtt_publicar_dato(const char *tipo, const char *valor) {
    const char *topic;
    if (strcmp(tipo, "humedad") == 0) { 
        topic = MQTT_TOPIC_HUM; // Tema de humedad
    } else if (strcmp(tipo, "temperatura") == 0) {
        topic = MQTT_TOPIC_TEMP;    // Tema de temperatura
    } else if (strcmp(tipo, "nivel") == 0) {
        topic = MQTT_TOPIC_NIVEL;   // Tema de nivel
    } else {
        ESP_LOGW(MQTT_TAG, "Tipo de dato desconocido: %s", tipo);
        return;
    }
    esp_mqtt_client_publish(mqtt_client, topic, valor, 0, 1, 0);    // Publicar el dato
}

// Función para publicar datos de calibración
void mqtt_publicar_calibracion(const char *etapa, uint32_t valor) {
    char msg[64];
    snprintf(msg, sizeof(msg), "{\"%s\": %lu}", etapa, (unsigned long)valor);
    esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_CALIB, msg, 0, 1, 0);
}
