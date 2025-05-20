// main.c
#include "adc.h"
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>

void app_main(void) {
    esp_log_level_set(TAG, ESP_LOG_INFO);

    cola_sensores    = xQueueCreate(15, sizeof(sensor_data_t));
    eventos_sensores = xEventGroupCreate();
    if (!cola_sensores || !eventos_sensores) {
        ESP_LOGE(TAG, "Error inicializando FreeRTOS");
        return;
    }
    wifi_init_sta();
    sensores_init();
    mqtt_app_start();

    // Calibración
    ESP_LOGI(TAG, "Calibrando sensor de humedad: punto seco...");
    calibrar_humedad();
    // Aquí usamos PRIu32 para uint32_t
    ESP_LOGI(TAG,
        "Calibración completada: raw_dry=%" PRIu32 ", raw_wet=%" PRIu32,
        raw_dry, raw_wet
    );

    xTaskCreatePinnedToCore(
        vTaskReadSensors, "LecturaSensores", 4096, NULL,
        configMAX_PRIORITIES - 2, NULL, 1
    );
    xTaskCreatePinnedToCore(
        vTaskProcessSensors, "Procesamiento", 4096, NULL,
        configMAX_PRIORITIES - 3, NULL, 1
    );

    ESP_LOGI(TAG, "Sistema de sensores iniciado");
}