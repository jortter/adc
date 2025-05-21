// adc.h
#ifndef ADC_H
#define ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

// Configuración ADC
#define ADC_WIDTH      ADC_WIDTH_BIT_12
#define ADC_ATTEN      ADC_ATTEN_DB_11

// Canales ADC (ajusta según el pin que uses)
#define CANAL_HUMEDAD   ADC1_CHANNEL_3   // GPIO4 
#define CANAL_TEMP      ADC1_CHANNEL_4   // GPIO5

// Evento nuevo dato
#define EVENTO_NUEVOS_DATOS (1 << 0)

extern const char *TAG;

typedef struct {
    float humedad_pct;
    float temperatura_c;
    TickType_t timestamp;
} sensor_data_t;

// Cola y grupo de eventos
extern QueueHandle_t      cola_sensores;
extern EventGroupHandle_t eventos_sensores;

// Valores raw para calibración (0..4095)
extern uint32_t raw_dry;
extern uint32_t raw_wet;

// Prototipos
void sensores_init(void);
void calibrar_humedad(void);
void vTaskReadSensors(void *pvParameters);
void vTaskProcessSensors(void *pvParameters);
void adc_set_mqtt_client(esp_mqtt_client_handle_t client);

#endif // ADC_H
