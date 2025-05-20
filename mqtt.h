#ifndef MQTT_H
#define MQTT_H

void mqtt_app_start(void);
void mqtt_publish_sensor_data(float humedad, float temperatura);

#endif // MQTT_H
