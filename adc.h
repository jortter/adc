// adc.h
#ifndef ADC_H
#define ADC_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

// Configuración ADC
#define ADC_WIDTH      ADC_WIDTH_BIT_12 // 12 bits
#define ADC_ATTEN      ADC_ATTEN_DB_11  // 11 dB

#define CANAL_HUMEDAD ADC1_CHANNEL_3    // GPIO4
#define CANAL_TEMP ADC1_CHANNEL_0      // GPIO1
#define CANAL_NIVEL ADC1_CHANNEL_1    // GPIO2

// Definición de la estructura de datos del sensor para almacenar los datos de los sensores 
typedef struct {
    float humedad_pct;
    float temperatura_c;
    float nivel_pct;
    TickType_t timestamp;
} sensor_data_t;

extern QueueHandle_t cola_sensores; // Cola para almacenar los datos procesados de los sensores
extern EventGroupHandle_t eventos_sensores; // Grupo de eventos para la sincronización de tareas
extern const int EVENTO_NUEVOS_DATOS;   // Evento para nuevos datos

void sensores_init(void);   // Inicializa los sensores y la cola de datos
void vTaskReadSensors(void *pvParameters);  // Tarea para leer los datos de los sensores
void vTaskProcessSensors(void *pvParameters);   // Tarea para procesar los datos de los sensores
void calibrar_humedad(void);    // Calibra el sensor de humedad

#endif // ADC_H