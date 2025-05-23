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

void mqtt_app_start(void);
void mqtt_publicar_dato(const char *tipo, const char *valor);
void mqtt_publicar_calibracion(const char *etapa, uint32_t valor);
#endif