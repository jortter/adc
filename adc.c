// adc.c
#include "adc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "mqtt.h"
#include <inttypes.h>
#include <string.h>

static const char *SENSOR_TAG = "SENSORES"; // Etiqueta para el registro de logs
const int EVENTO_NUEVOS_DATOS = (1 << 0);   // Evento para nuevos datos
// Definición de la estructura de datos del sensor
QueueHandle_t cola_sensores = NULL;
EventGroupHandle_t eventos_sensores = NULL;
static esp_adc_cal_characteristics_t *adc_chars = NULL;
static uint32_t raw_dry = 0xFFFF;
static uint32_t raw_wet = 0;

void sensores_init(void) {
    adc1_config_width(ADC_WIDTH);   // Configuración de la resolución del ADC
    adc1_config_channel_atten(CANAL_HUMEDAD, ADC_ATTEN);    // Sensor de humedad
    adc1_config_channel_atten(CANAL_TEMP, ADC_ATTEN);       // Sensor de temperatura
    adc1_config_channel_atten(CANAL_NIVEL, ADC_ATTEN);     // Sensor de nivel

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));   // Reservar memoria para las características del ADC
    if (adc_chars == NULL) {
        ESP_LOGE(SENSOR_TAG, "Error al reservar memoria para las características del ADC");
        return;
    }

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, adc_chars);    // Caracterizar el ADC

    cola_sensores = xQueueCreate(15, sizeof(sensor_data_t));    // Crear la cola para los datos del sensor
    eventos_sensores = xEventGroupCreate();   // Crear el grupo de eventos
}

void calibrar_humedad(void) {
    uint32_t r; // Variable para almacenar el valor leído del ADC
    ESP_LOGI(SENSOR_TAG, ">> Mantén el sensor SECO (por ejemplo, en algodón seco)");
    vTaskDelay(pdMS_TO_TICKS(10000));   // Esperar 10 segundos
    uint32_t min_raw = 4095;    // Valor máximo posible para el ADC
    for (int i = 0; i < 16; i++) {  // Leer el ADC 16 veces
        r = adc1_get_raw(CANAL_HUMEDAD);    // Leer el valor del ADC
        if (r < min_raw) min_raw = r;   // Guardar el valor mínimo
        vTaskDelay(pdMS_TO_TICKS(50));  // Esperar 50 ms entre lecturas
    }
    raw_dry = min_raw;  // Guardar el valor mínimo como el valor seco
    mqtt_publicar_calibracion("dry", raw_dry);  // Publicar el valor seco por MQTT
    
    // Lo mismo para el valor húmedo
    ESP_LOGI(SENSOR_TAG, ">> Ahora SUMERGE el sensor completamente en agua");
    vTaskDelay(pdMS_TO_TICKS(10000));
    uint32_t max_raw = 0;
    for (int i = 0; i < 16; i++) {
        r = adc1_get_raw(CANAL_HUMEDAD);
        if (r > max_raw) max_raw = r;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    raw_wet = max_raw;
    mqtt_publicar_calibracion("wet", raw_wet);
    // Si el valor seco es menor que el húmedo, intercambiamos los valores porque se han tomado al revés
    if (raw_dry < raw_wet) {
        uint32_t tmp = raw_dry;
        raw_dry = raw_wet;
        raw_wet = tmp;
    }
}

// Tarea para leer los sensores y enviar los datos a la cola de sensores
void vTaskReadSensors(void *pvParameters) {
    sensor_data_t d;    // Estructura para almacenar los datos del sensor
    uint32_t raw_h, raw_t, raw_n;   // Variables para almacenar los valores leídos del ADC
    for (;;) {
        raw_h = adc1_get_raw(CANAL_HUMEDAD);
        // Calibrar el valor de humedad
        if (raw_dry > raw_wet) {
            int32_t delta = (int32_t)raw_dry - (int32_t)raw_h;
            float pct = (float)delta * 100.0f / (float)(raw_dry - raw_wet); // Calcular el porcentaje
            d.humedad_pct = (pct < 0 ? 0 : (pct > 100 ? 100 : pct));    // Limitar el valor entre 0 y 100
        } else {
            uint32_t mv = esp_adc_cal_raw_to_voltage(raw_h, adc_chars); // Convertir a milivoltios
            d.humedad_pct = mv * (100.0f / 4095.0f);    // Calcular el porcentaje
        }
        raw_t = adc1_get_raw(CANAL_TEMP);
        uint32_t mv_t = esp_adc_cal_raw_to_voltage(raw_t, adc_chars);   // Convertir a milivoltios
        d.temperatura_c = ((float)mv_t - 500.0f) / 10.0f;   // Calcular la temperatura en grados Celsius para el TMP36
        d.timestamp = xTaskGetTickCount();  // Obtener la marca de tiempo

        raw_n = adc1_get_raw(CANAL_NIVEL);  // Leer el valor del ADC para el sensor de nivel
        d.nivel_pct = (raw_n * 100.0f) / 4095.0f;   // Calcular el porcentaje de nivel

        if (xQueueSend(cola_sensores, &d, pdMS_TO_TICKS(20)) == pdTRUE) {
            xEventGroupSetBits(eventos_sensores, EVENTO_NUEVOS_DATOS);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));   // Esperar 1 segundo antes de leer nuevamente
    }
}

void vTaskProcessSensors(void *pvParameters) {
    sensor_data_t r;    // Estructura para almacenar los datos del sensor
    char payload[64];   // Buffer para el payload MQTT
    for (;;) {
        // Esperar a que haya nuevos datos en la cola
        if (xQueueReceive(cola_sensores, &r, portMAX_DELAY) == pdTRUE) {
            snprintf(payload, sizeof(payload), "%.2f", r.humedad_pct);  // Formatear la humedad
            mqtt_publicar_dato("humedad", payload); // Publicar la humedad
            snprintf(payload, sizeof(payload), "%.2f", r.temperatura_c);    // Formatear la temperatura
            mqtt_publicar_dato("temperatura", payload); // Publicar la temperatura
            snprintf(payload, sizeof(payload), "%.2f", r.nivel_pct);    // Formatear el nivel
            mqtt_publicar_dato("nivel", payload);   // Publicar el nivel
        }
        vTaskDelay(pdMS_TO_TICKS(1000));   // Esperar 1 segundo antes de procesar nuevos datos
    }
}