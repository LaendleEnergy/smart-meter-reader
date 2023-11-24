#include "my_mqtt_client.h"

static void (*mqtt_transport_send_function)(void * data, size_t length);

void mqtt_set_transport_function(void (*transport)(void * data, size_t length)){
    mqtt_transport_send_function = transport;
}


// Function to build and send MQTT connect packet
void mqtt_connect() {

    // Send the connect packet via the transport layer
    uint8_t buffer[12] = {0};
    buffer[0] = 0x10;
    buffer[1] = 12;
    buffer[2] = 0;
    buffer[3] = 4;
    buffer[4] = 'M';
    buffer[5] = 'Q';
    buffer[6] = 'T';
    buffer[7] = 'T';
    buffer[8] = 4;
    buffer[9] = 0x2;
    buffer[10] = 0;
    buffer[11] = 60;
    if(mqtt_transport_send_function!=NULL){
        mqtt_transport_send_function(buffer, sizeof(buffer));
    }   
    
}

void mqtt_publish(void * data, size_t length){
    uint8_t mqttPublishMessage[12] = {
            0x30,       // MQTT Control Packet Type: PUBLISH (0x30)
            12,       // Remaining Length: 14 in decimal
            0x00, 0x03, // Topic Name Length: 3
            0x61, 0x2F, 0x62, // Topic Name: a/b
            0x48, 0x65, 0x6C, 0x6C, 0x6F, // Payload: "Hello"
    };
    if(mqtt_transport_send_function!=NULL){
        mqtt_transport_send_function(mqttPublishMessage, sizeof(mqttPublishMessage));
    }
}

