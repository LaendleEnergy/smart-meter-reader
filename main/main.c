#include <stdio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <mbedtls/sha256.h>
// #include "mbedtls/md.h"
#include <math.h>
#include <freertos/queue.h>

// MBUS Defines
#define MBUS_UART UART_NUM_1
#define MBUS_RX 5
#define MBUS_BUFFER_SIZE 1024


#define MAX_FRAME_COUNT 2
#define KEY_SIZE 16
#define START_VECTOR_SIZE 12
#define PLAINTEXT_SIZE 512
#define ENCRYPTED_SIZE 64

//Modem Defines
#define MODEM_UART UART_NUM_0
#define MODEM_RX 20
#define MODEM_TX 21
#define MODEM_RI 3
#define MODEM_PWRKEY 6
#define MODEM_DTR 7
#define MODEM_BUFFER_SIZE 2048
#define MODEM_SEND_BUFFER_SIZE 512

#define MODEM_APN "iot.1nce.net"
#define SERVER "85.215.60.151"//"45.145.224.10"
#define PORT "4433"//"1883"

#define AUTH_SUCCESS 0x22
#define AUTH_FAILED 0x11
#define DATA_SUCCESS 0x44
#define DATA_FAILED 0x33

#include "mbus_minimal.h"
#include "dlms.h"
#include "kaifa.h"

#include "sim7020e.h"

#include "my_mqtt_client.h"


uint8_t plaintext_data[PLAINTEXT_SIZE] = {0};
uint8_t encrypted_data[PLAINTEXT_SIZE] = {0};

uint8_t key[16] = {0x32, 0x69, 0x31, 0x63, 0x79, 0x79, 0x45, 0x6C, 0x59, 0x37, 0x34, 0x44, 0x73, 0x6D, 0x33, 0x75};

uint8_t udp_key[16] = {0xb2, 0x0d, 0x7a, 0x8c, 0x55, 0x1e, 0x3f, 0x9a,0x64, 0x0b, 0x2f, 0x78, 0xd1, 0x5a, 0x0c, 0x91};

mbus_packet_t mbus_list[MAX_FRAME_COUNT];
dlms_data_t dlms;
kaifa_data_t kaifa, kaifa_old;

QueueHandle_t data_queue;

uint8_t usdp_state = 0;


void to_hex_string(char * hex_buffer, uint16_t hex_buffer_size, uint8_t * buffer, uint16_t buffer_size){
    memset(hex_buffer, 0, hex_buffer_size);
    char * buf_ptr = hex_buffer;
    for(uint16_t i = 0; i < buffer_size && i< hex_buffer_size; i++){
        buf_ptr += sprintf(buf_ptr, "%02X", buffer[i]);
    }
}






void mbus_thread(void * param){
    mbus_init();
    memset(&kaifa_old, 0, sizeof(kaifa_data_t));
    for(;;){
        size_t mbus_count = mbus_receive_multiple(mbus_list, MAX_FRAME_COUNT);

        memset(&dlms, 0, sizeof(dlms_data_t));
        dlms_set_data(&dlms, mbus_list, mbus_count);

        size_t plaintext_size = decrypt_aes_gcm(key, sizeof(key), dlms.start_vector, sizeof(dlms.start_vector), dlms.apdu, dlms.apdu_length, plaintext_data, sizeof(plaintext_data));

        memset(&kaifa, 0, sizeof(kaifa_data_t));
        parse_obis_codes(&kaifa, plaintext_data, plaintext_size);
        char json_buffer[1000] = {0};
        kaifa_data_to_json(&kaifa, json_buffer);
        // printf("%s\n", json_buffer);

        memcpy(&kaifa_old, &kaifa, sizeof(kaifa_data_t));
        xQueueSend(data_queue, &kaifa, 100/portTICK_PERIOD_MS);

        // if(kaifa_data_differnce(&kaifa, &kaifa_old, 0.05)){
            
        // }

    }
}





void compute_challenge_response(uint8_t * response, uint8_t * challenge, uint8_t * key){
    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);
    mbedtls_sha256_update(&sha256, (uint8_t*)challenge, 16);
    mbedtls_sha256_update(&sha256, key, 16);

    mbedtls_sha256_finish(&sha256, response);
}


bool usdp_is_authenticated(){
    return usdp_state;
}

bool usdp_authenticate(uint8_t * mac, uint8_t * session_key, uint8_t * challenge){
    uint8_t auth_frame[7] = {0xFF, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]};
    uint8_t start_vector[12] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B};

    for(uint8_t i = 0; i < 5; i++){
        network_send_data(auth_frame, 7);
        ESP_LOGI("USDP","Sending Authframe");

        memset(challenge, 0, 16);
        // network_read_data_blocking(challenge, 16);
        if(network_read_data_blocking_with_timeout(challenge, 16, 3000)<0){
            ESP_LOGE("USDP","Receive Challenge timeout");
            continue;
        }

        ESP_LOGI("USDP","Challenge received");

        uint8_t response[33] = {0};
        response[0] = 0xFF;

        uint8_t tag[16] = {0};
        encrypt_aes_gcm(udp_key, 16, start_vector, 12, challenge, 16, session_key, 16, tag, 0);

        // derive_key(key, 16, challenge, 16);
        compute_challenge_response(response+1, challenge, session_key);
        ESP_LOG_BUFFER_HEX("USDP Challenge response", response, 33);
        network_send_data(response, 33);

        uint8_t server_response = 0;
        // network_read_data_blocking(&server_response, 1);

        if(network_read_data_blocking_with_timeout(&server_response, 1, 3000)<0){
            ESP_LOGE("USDP","Server response timeout");
            continue;
        }
        ESP_LOGI("USDP Server Response", "0x%02X",server_response);

        if(server_response==AUTH_FAILED){
            ESP_LOGE("USDP","Authentication failed");
            continue;
        }

        usdp_state = 1;

        return true;
    }

    usdp_state = 0;
    return false;
    
}

void modem_thread(void * param){

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buffer[13] = {0};
    sprintf(buffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    uint8_t usdp_iv[12] = {0};
    uint8_t challenge[16] = {0};
    uint64_t frame_counter = 0;
    uint8_t frame_counter_offset = 0;
    uint8_t tag[16] = {0};
    uint8_t usdp_frame[42] = {0};
    uint8_t session_key[16] = {0};

    sim7020e_init();
    sim7020e_handle_connection();

    for(;;){
        if(!sim7020e_is_connected()){
            sim7020e_connect_udp(SERVER, PORT);
        }else{ // connected and ready to authenticate
            if(!usdp_is_authenticated()){
                if(usdp_authenticate(mac, session_key, challenge)){
                    ESP_LOGI("USDP","Authenticated");
                
                    frame_counter = (uint64_t)challenge[11]<<56 | (uint64_t)challenge[10]<<48 | (uint64_t)challenge[9]<<40 | (uint64_t)challenge[8]<<32 | (uint64_t)challenge[7]<<24 | (uint64_t)challenge[6]<<16 | (uint64_t)challenge[5]<<8 | challenge[4];
                    frame_counter_offset = frame_counter % 100;
                }
            }else{ // Authenticated and ready to send data
                kaifa_data_t data = {0};
                if(xQueuePeek(data_queue, &data, 100/portTICK_PERIOD_MS)==pdTRUE){
                    
                    frame_counter+=frame_counter_offset;
                    memcpy(usdp_iv, &frame_counter, 8);
                    memcpy(usdp_iv+8, challenge, 4);
                    
                    ESP_LOGI("USDP Frame Counter", "%lld", frame_counter);
                    ESP_LOG_BUFFER_HEX("USDP IV", usdp_iv, 12);
                    ESP_LOG_BUFFER_HEX("USDP key", udp_key, 16);

                    uint8_t kaifa_buffer[36] = {0};
                    kaifa_data_to_buffer(kaifa_buffer, 36, &data);

                    ESP_LOG_BUFFER_HEX("USDP Data", kaifa_buffer, 36);
                    
                    memset(encrypted_data, 0,  ENCRYPTED_SIZE);
                    size_t length = encrypt_aes_gcm(session_key, sizeof(session_key), usdp_iv, sizeof(usdp_iv), kaifa_buffer, sizeof(kaifa_buffer), encrypted_data, sizeof(encrypted_data), tag, sizeof(tag));
                    ESP_LOG_BUFFER_HEX("USDP TAG", tag, 16);
                    usdp_frame[0] = 0x0F; //Frame Type: Data
                    usdp_frame[1] = 0;    //protocol Version: 0
                    memcpy(usdp_frame+2, encrypted_data, length); //kaifa data
                    memcpy(usdp_frame+2+length, tag, 4);          // 4 byte tag for validation;
                    ESP_LOG_BUFFER_HEX("USDP", usdp_frame, 42);

                    uint8_t server_response = 0;
                    do{
                        ESP_LOGI("USDP", "Sending data");
                        network_send_data(usdp_frame, sizeof(usdp_frame));
                        if(network_read_data_blocking_with_timeout(&server_response, 1, 10000)<0){
                            ESP_LOGE("USDP", "Response timeout");
                        }
                        ESP_LOGI("USDP", "Response received %02X", server_response);
                    }while(server_response!=0x44);

                    xQueueReceive(data_queue, &data, 0);
                    
                }
            }
        }

    }

    
    
}

void app_main(void){

    data_queue = xQueueCreate(100, sizeof(kaifa_data_t));

    xTaskCreate(mbus_thread, "mbus thread", 2048, NULL, 0, NULL);
    xTaskCreate(modem_thread, "modem thread", 4096, NULL, 0, NULL);


    // for(;;){
    //     kaifa_data_t kaifa;
    //     kaifa_generate_test_data(&kaifa);
    //     ESP_LOGI("MAIN", "generated test data");

    //     xQueueSend(data_queue, &kaifa, 100/portTICK_PERIOD_MS);
    //     vTaskDelay(5000/portTICK_PERIOD_MS);
    // }
}
