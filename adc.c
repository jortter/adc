// adc.c
#include "adc.h"
#include "mqtt.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/task.h"
#include "esp_log.h"

QueueHandle_t      cola_sensores    = NULL;
EventGroupHandle_t eventos_sensores = NULL;
const char       *TAG               = "SENSORES";

static esp_adc_cal_characteristics_t *adc_chars = NULL;
// Calibración en dos puntos
uint32_t raw_dry = 0xFFFF;  // se actualizará en calibrar_humedad()
uint32_t raw_wet = 0;

void sensores_init(void) {
    // Configurar ADC
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(CANAL_HUMEDAD, ADC_ATTEN);
    adc1_config_channel_atten(CANAL_TEMP,    ADC_ATTEN);

    // Calibración interna ESP32
    adc_chars = calloc(1, sizeof(*adc_chars));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, adc_chars);

    // Si no existen aún, crea cola y grupo de eventos
    if (!cola_sensores)    cola_sensores    = xQueueCreate(15, sizeof(sensor_data_t));
    if (!eventos_sensores) eventos_sensores = xEventGroupCreate();
}

// Lee 16 muestras en seco, toma el mínimo; luego 16 en agua, toma el máximo
void calibrar_humedad(void) {
    uint32_t r;

    ESP_LOGI(TAG, ">> Mantén el sensor SECO (por ejemplo, en algodón seco)");
    vTaskDelay(pdMS_TO_TICKS(10000));  // espera 3 segundos

    uint32_t min_raw = 4095;
    for (int i = 0; i < 16; i++) {
        r = adc1_get_raw(CANAL_HUMEDAD);
        if (r < min_raw) min_raw = r;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    raw_dry = min_raw;
    ESP_LOGI(TAG, "Lectura en seco (raw_dry) = %" PRIu32, raw_dry);

    ESP_LOGI(TAG, ">> Ahora SUMERGE el sensor completamente en agua");
    vTaskDelay(pdMS_TO_TICKS(10000));  // espera 5 segundos

    uint32_t max_raw = 0;
    for (int i = 0; i < 16; i++) {
        r = adc1_get_raw(CANAL_HUMEDAD);
        if (r > max_raw) max_raw = r;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    raw_wet = max_raw;
    ESP_LOGI(TAG, "Lectura en agua (raw_wet) = %" PRIu32, raw_wet);

    // ✅ Corregir si están al revés
    if (raw_dry < raw_wet) {
        uint32_t tmp = raw_dry;
        raw_dry = raw_wet;
        raw_wet = tmp;
        ESP_LOGW(TAG, "¡Valores invertidos! Corrigiendo raw_dry y raw_wet");
    }
}



void vTaskReadSensors(void *pvParameters) {
    sensor_data_t d;
    uint32_t raw_h, raw_t;

    while (1) {
        // --- Humedad raw ---
        raw_h = adc1_get_raw(CANAL_HUMEDAD);

        // Si está calibrado, escala entre raw_dry..raw_wet
        if (raw_dry > raw_wet) {
            int32_t delta = (int32_t)raw_dry - (int32_t)raw_h;
            float pct = (float)delta * 100.0f / (float)(raw_dry - raw_wet);
            d.humedad_pct = (pct < 0 ? 0 : (pct > 100 ? 100 : pct));
        } else {
            // fallback: sin calibración
            uint32_t mv = esp_adc_cal_raw_to_voltage(raw_h, adc_chars);
            d.humedad_pct = mv * (100.0f / 4095.0f);
        }

        // --- Temperatura raw + voltaje ---
        raw_t = adc1_get_raw(CANAL_TEMP);
        uint32_t mv_t = esp_adc_cal_raw_to_voltage(raw_t, adc_chars);
        d.temperatura_c = mv_t * (330.0f / 4095.0f);

        d.timestamp = xTaskGetTickCount();

        if (xQueueSend(cola_sensores, &d, pdMS_TO_TICKS(20)) == pdTRUE) {
            xEventGroupSetBits(eventos_sensores, EVENTO_NUEVOS_DATOS);
        } else {
            ESP_LOGW(TAG, "Cola llena, descartando muestra");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTaskProcessSensors(void *pvParameters) {
    sensor_data_t r;
    while (1) {
        if (xQueueReceive(cola_sensores, &r, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG,
                "Humedad: %.1f%%   Temp: %.1f°C   @%lu",
                r.humedad_pct,
                r.temperatura_c,
                (unsigned long)r.timestamp
            );
            mqtt_publish_sensor_data(r.humedad_pct, r.temperatura_c);
        }
    }
}
