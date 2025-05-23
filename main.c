// main.c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"

#include "adc.h"
#include "wifi.h"
#include "mqtt.h"

static uint32_t raw_dry = 0xFFFF;   // Valor mínimo leído del ADC para el sensor seco
static uint32_t raw_wet = 0;      // Valor máximo leído del ADC para el sensor húmedo

static const char *TAG = "Main";    // Etiqueta para el registro de logs

void app_main(void) {
    ESP_LOGI(TAG, "Inicializando WiFi...");
    wifi_init_sta();    // Inicializar WiFi
    vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar conexión WiFi

    ESP_LOGI(TAG, "Inicializando MQTT...");
    mqtt_app_start();   // Inicializar MQTT
    vTaskDelay(pdMS_TO_TICKS(3000)); // Esperar conexión MQTT

    ESP_LOGI(TAG, "Inicializando sensores...");
    sensores_init();    // Inicializar sensores

    ESP_LOGI(TAG, "Calibrando sensor de humedad: punto seco...");
    calibrar_humedad(); // Calibrar el sensor de humedad
    ESP_LOGI(TAG,
        "Calibración completada: raw_dry=%" PRIu32 ", raw_wet=%" PRIu32,
        raw_dry, raw_wet
    );

    ESP_LOGI(TAG, "Creando tareas...");
    xTaskCreate(vTaskReadSensors, "ReadSensors", 4096, NULL, 5, NULL);  // Crear tarea para leer sensores
    xTaskCreate(vTaskProcessSensors, "ProcessSensors", 4096, NULL, 5, NULL);    // Crear tarea para procesar sensores

    ESP_LOGI(TAG, "Sistema iniciado correctamente.");
    
    // Mantener el programa corriendo si no hay más lógica en app_main
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
