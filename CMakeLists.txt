idf_component_register(SRCS  "main.c" "adc.c" "wifi.c" "mqtt.c" 
                       INCLUDE_DIRS ".")

set(COMPONENT_REQUIRES 
    esp_netif 
    esp_wifi 
    mqtt 
    nvs_flash 
    freertos 
    lwip 
    log
)