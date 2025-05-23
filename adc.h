#ifndef ADC_H
#define ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

typedef struct {
    float humedad_pct;
    float temperatura_c;
    TickType_t timestamp;
} sensor_data_t;

extern QueueHandle_t cola_sensores;
extern EventGroupHandle_t eventos_sensores;
extern const int EVENTO_NUEVOS_DATOS;

void sensores_init(void);
void vTaskReadSensors(void *pvParameters);
void vTaskProcessSensors(void *pvParameters);
void calibrar_humedad(void);

#endif // ADC_H