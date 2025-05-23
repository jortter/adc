#include "adc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "mqtt.h"
#include <inttypes.h>
#include <string.h>

#define ADC_WIDTH ADC_WIDTH_BIT_12
#define ADC_ATTEN ADC_ATTEN_DB_6
#define CANAL_HUMEDAD ADC1_CHANNEL_4
#define CANAL_TEMP ADC1_CHANNEL_5

static const char *SENSOR_TAG = "SENSORES";
const int EVENTO_NUEVOS_DATOS = (1 << 0);
QueueHandle_t cola_sensores = NULL;
EventGroupHandle_t eventos_sensores = NULL;
static esp_adc_cal_characteristics_t *adc_chars = NULL;
static uint32_t raw_dry = 0xFFFF;
static uint32_t raw_wet = 0;

void sensores_init(void) {
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(CANAL_HUMEDAD, ADC_ATTEN);
    adc1_config_channel_atten(CANAL_TEMP, ADC_ATTEN);

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, adc_chars);

    cola_sensores = xQueueCreate(15, sizeof(sensor_data_t));
    eventos_sensores = xEventGroupCreate();
}

void calibrar_humedad(void) {
    uint32_t r;
    ESP_LOGI(SENSOR_TAG, ">> Sensor seco...");
    vTaskDelay(pdMS_TO_TICKS(10000));
    uint32_t min_raw = 4095;
    for (int i = 0; i < 16; i++) {
        r = adc1_get_raw(CANAL_HUMEDAD);
        if (r < min_raw) min_raw = r;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    raw_dry = min_raw;
    mqtt_publicar_calibracion("dry", raw_dry);

    ESP_LOGI(SENSOR_TAG, ">> Sensor mojado...");
    vTaskDelay(pdMS_TO_TICKS(10000));
    uint32_t max_raw = 0;
    for (int i = 0; i < 16; i++) {
        r = adc1_get_raw(CANAL_HUMEDAD);
        if (r > max_raw) max_raw = r;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    raw_wet = max_raw;
    mqtt_publicar_calibracion("wet", raw_wet);

    if (raw_dry < raw_wet) {
        uint32_t tmp = raw_dry;
        raw_dry = raw_wet;
        raw_wet = tmp;
    }
}

void vTaskReadSensors(void *pvParameters) {
    sensor_data_t d;
    uint32_t raw_h, raw_t;
    for (;;) {
        raw_h = adc1_get_raw(CANAL_HUMEDAD);
        if (raw_dry > raw_wet) {
            int32_t delta = (int32_t)raw_dry - (int32_t)raw_h;
            float pct = (float)delta * 100.0f / (float)(raw_dry - raw_wet);
            d.humedad_pct = (pct < 0 ? 0 : (pct > 100 ? 100 : pct));
        } else {
            uint32_t mv = esp_adc_cal_raw_to_voltage(raw_h, adc_chars);
            d.humedad_pct = mv * (100.0f / 4095.0f);
        }
        raw_t = adc1_get_raw(CANAL_TEMP);
        uint32_t mv_t = esp_adc_cal_raw_to_voltage(raw_t, adc_chars);
        d.temperatura_c = ((float)mv_t - 500.0f) / 10.0f;
        d.timestamp = xTaskGetTickCount();

        if (xQueueSend(cola_sensores, &d, pdMS_TO_TICKS(20)) == pdTRUE) {
            xEventGroupSetBits(eventos_sensores, EVENTO_NUEVOS_DATOS);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vTaskProcessSensors(void *pvParameters) {
    sensor_data_t r;
    char payload[64];
    for (;;) {
        if (xQueueReceive(cola_sensores, &r, portMAX_DELAY) == pdTRUE) {
            snprintf(payload, sizeof(payload), "%.2f", r.humedad_pct);
            mqtt_publicar_dato("humedad", payload);
            snprintf(payload, sizeof(payload), "%.2f", r.temperatura_c);
            mqtt_publicar_dato("temperatura", payload);
        }
    }
}