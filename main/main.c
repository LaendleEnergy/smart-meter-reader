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
#include "esp_efuse.h"


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

#include "mbus_minimal.h"
#include "dlms.h"
#include "kaifa.h"

#include "sim7020e.h"

#include "usdp.h"




uint8_t plaintext_data[PLAINTEXT_SIZE] = {0};
uint8_t encrypted_data[PLAINTEXT_SIZE] = {0};

uint8_t key[16] = {0x32, 0x69, 0x31, 0x63, 0x79, 0x79, 0x45, 0x6C, 0x59, 0x37, 0x34, 0x44, 0x73, 0x6D, 0x33, 0x75};

uint8_t secret_key[16] = {0xb2, 0x0d, 0x7a, 0x8c, 0x55, 0x1e, 0x3f, 0x9a,0x64, 0x0b, 0x2f, 0x78, 0xd1, 0x5a, 0x0c, 0x91};

mbus_packet_t mbus_list[MAX_FRAME_COUNT];
dlms_data_t dlms;
kaifa_data_t kaifa, kaifa_old;

QueueHandle_t data_queue;


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









void modem_thread(void * param){

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buffer[13] = {0};
    sprintf(buffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    

    sim7020e_init();
    sim7020e_handle_connection();
    usdp_init(network_read_data_blocking_with_timeout, network_send_data);

    for(;;){
        if(!sim7020e_is_connected()){
            sim7020e_connect_udp(SERVER, PORT);
        }else{ // connected and ready to authenticate
            if(!usdp_is_authenticated()){
                usdp_authenticate(mac, secret_key);
            }else{ // Authenticated and ready to send data
                kaifa_data_t data = {0};
                if(xQueuePeek(data_queue, &data, 100/portTICK_PERIOD_MS)==pdTRUE){
                    
                    uint8_t kaifa_buffer[36] = {0};
                    kaifa_data_to_buffer(kaifa_buffer, 36, &data);
                    ESP_LOG_BUFFER_HEX("USDP Data", kaifa_buffer, 36);

                    if(usdp_send_data_encrypted(kaifa_buffer, sizeof(kaifa_buffer))){
                        xQueueReceive(data_queue, &data, 0);
                    }
                    
                }
            }
        }

    }

}

void app_main(void){ 


    // ESP_LOGI("HMAC", "Purpose: %d", esp_efuse_get_key_purpose(EFUSE_BLK_KEY5));

    // uint8_t hmac_key[32] = {0};
    // uint8_t comp[54] = {0};
    // esp_err_t err = esp_efuse_read_block(EFUSE_BLK_KEY5, comp, 0, 64*8);
    // ESP_LOG_BUFFER_HEX("HMAC Key read", comp, 64);


    // if(esp_efuse_block_is_empty(EFUSE_BLK_KEY5) && esp_efuse_key_block_unused(EFUSE_BLK_KEY5)){
    //     esp_fill_random(hmac_key, 32);
    //     ESP_LOG_BUFFER_HEX("HMAC Key gen", hmac_key, 32);
    //     esp_efuse_write_key(EFUSE_BLK_KEY3, ESP_EFUSE_KEY_PURPOSE_HMAC_UP, hmac_key, sizeof(hmac_key));

    //     esp_err_t err = esp_efuse_read_block(EFUSE_BLK_KEY5, comp, 0, 32*8);
    //     ESP_LOG_BUFFER_HEX("HMAC Key read", comp, 32);
    // }
    

    



    data_queue = xQueueCreate(100, sizeof(kaifa_data_t));

    xTaskCreate(mbus_thread, "mbus thread", 2048, NULL, 0, NULL);
    xTaskCreate(modem_thread, "modem thread", 4096, NULL, 0, NULL);




    for(;;){
        kaifa_data_t kaifa;
        kaifa_generate_test_data(&kaifa);
        ESP_LOGI("MAIN", "generated test data");

        xQueueSend(data_queue, &kaifa, 100/portTICK_PERIOD_MS);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}
