// main.c
#include "adc.h"
#include "wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>

void app_main(void) {
    nvs_flash_erase();  // ⚠ Esto borra TODO (certificados, claves, configuraciones previas)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_log_level_set(TAG, ESP_LOG_INFO);

    cola_sensores    = xQueueCreate(15, sizeof(sensor_data_t));
    eventos_sensores = xEventGroupCreate();
    if (!cola_sensores || !eventos_sensores) {
        ESP_LOGE(TAG, "Error inicializando FreeRTOS");
        return;
    }

    wifi_init_sta();        // 1. Conectar a WiFi
    sensores_init();       // 2. Inicializar ADC y FreeRTOS
    //mqtt_app_start();      // 3. Conectar y publicar MQTT

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
    //mqtt_app_start();       // 2. Conectar y publicar MQTT
}