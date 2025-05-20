idf_component_register(
    SRCS 
        "main.c"
        "adc.c"
        "wifi.c"
        "mqtt.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver        # para driver/adc.h
        esp_adc_cal   # para esp_adc_cal.h
        esp_wifi
        esp_netif
        esp_event
        nvs_flash
        mqtt          # para el esp-mqtt
)