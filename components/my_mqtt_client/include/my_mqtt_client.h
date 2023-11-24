#ifndef MQTT_CLIENT
#define MQTT_CLIENT

#include <stdlib.h>
#include <stdint.h>

#include <esp_log.h>

typedef struct {
    uint8_t header; //0x10
    uint8_t remaining_length; //12
    uint16_t protocol_name_length; //4
    char protocol_name[4]; // "MQTT"
    uint8_t protocol_level; //4
    uint8_t connect_flags; //0x02
    uint16_t keep_alive; //60s
}MQTTConnectPacket;

void mqtt_set_transport_function(void (*transport)(void * data, size_t length));

void mqtt_publish(void * data, size_t length);

void mqtt_receive(void * data, size_t length);

void mqtt_connect();

#endif

