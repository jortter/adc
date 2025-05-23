// mqtt.h
#ifndef MQTT_H
#define MQTT_H
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


#define MQTT_BROKER_URI         "mqtt://broker.emqx.io:1883"        // URI del broker MQTT
#define MQTT_TOPIC_ESTADO       "UPV/PR2/2-08/sensor/estado"        // Tema para el estado del sensor
#define MQTT_TOPIC_COMMANDS     "UPV/PR2/2-08/sensor/commands"      // Tema para comandos
#define MQTT_TOPIC_TEMP         "UPV/PR2/2-08/sensor/temperatura"   // Tema para temperatura
#define MQTT_TOPIC_HUM          "UPV/PR2/2-08/sensor/humedad"       // Tema para humedad
#define MQTT_TOPIC_NIVEL        "UPV/PR2/2-08/sensor/nivel"         // Tema para nivel
#define MQTT_TOPIC_CALIB        "UPV/PR2/2-08/sensor/calibracion"   // Tema para calibración


void mqtt_app_start(void);  // Inicia el cliente MQTT
void mqtt_publicar_dato(const char *tipo, const char *valor);   // Publica datos en el tema correspondiente
void mqtt_publicar_calibracion(const char *etapa, uint32_t valor);  // Publica datos de calibración
#endif  // MQTT_H