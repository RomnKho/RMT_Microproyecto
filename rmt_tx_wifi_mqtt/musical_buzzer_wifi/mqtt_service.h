/**
 * @file    mqtt_service.h 
 * @brief   Header file for mqtt service
 **/

#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

/* ===== EXPORTED VARIABLES ===== */
extern QueueHandle_t received_data_mqtt_queue;

/* ===== EXPORTED FUNCTIONS ===== */
void mqtt_service_init(const char *uri);

int  mqtt_service_publish(const char *topic, const char *data, int len);

int  mqtt_service_subscribe(const char *topic);

bool mqtt_service_is_connected(void);

#endif

/* ===== END OF FILE ===== */