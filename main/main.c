#include <stdio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <string.h>
#include <stdlib.h>
#include <esp_mac.h>
#include <esp_random.h>
#include <mbedtls/sha256.h>
#include <math.h>
#include <freertos/queue.h>
#include "esp_efuse.h"
#include <nvs_flash.h>
#include <nvs.h>


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

#include "micro_guard_udp.h"
#include "utils.h"

typedef struct {
    uint8_t request[256];
    uint8_t size;
}data_request_t;

uint8_t plaintext_data[PLAINTEXT_SIZE] = {0};
uint8_t encrypted_data[PLAINTEXT_SIZE] = {0};

bool meter_key_needed = true;

uint8_t secret_key[16] = {0};


mbus_packet_t mbus_list[MAX_FRAME_COUNT];
dlms_data_t dlms;
kaifa_data_t kaifa, kaifa_old;

QueueHandle_t data_queue, request_queue, meter_key_queue;

void save_meter_key(uint8_t * meter_key){
    nvs_handle_t nvs;
    nvs_flash_init();
    nvs_open("key_storage", NVS_READWRITE, &nvs);
    nvs_set_blob(nvs, "meter_key", meter_key, 16);
    nvs_close(nvs);
}

void get_meter_key(){
    nvs_handle_t nvs;
    nvs_flash_init();
    nvs_open("key_storage", NVS_READWRITE, &nvs);
    size_t key_len = 0;
    esp_err_t err = nvs_get_blob(nvs, "meter_key", NULL, &key_len);
    if(err!=ESP_OK || key_len==0){
        data_request_t key_request;
        memcpy(key_request.request, "key", 3);
        key_request.size = 3;
        xQueueSend(request_queue, &key_request, 100/portTICK_PERIOD_MS);
        return;
    }
    uint8_t meter_key[16] = {0};
    nvs_get_blob(nvs, "meter_key", meter_key, &key_len);
    xQueueSend(meter_key_queue, meter_key, 100/portTICK_PERIOD_MS);
    nvs_close(nvs);
    //ESP_LOG_BUFFER_HEX("METER KEY", meter_key, key_len);
}

void mbus_thread(void * param){
    mbus_init();
    memset(&kaifa_old, 0, sizeof(kaifa_data_t));

    uint8_t meter_key[16] = {0};
    while(meter_key_needed){
        if(xQueueReceive(meter_key_queue, meter_key, 100/portTICK_PERIOD_MS)==pdTRUE){
            save_meter_key(meter_key);
            meter_key_needed=false;
            //ESP_LOG_BUFFER_HEX("METER KEY", meter_key, 16);
        }
    }
    for(;;){
        size_t mbus_count = mbus_receive_multiple(mbus_list, MAX_FRAME_COUNT);

        memset(&dlms, 0, sizeof(dlms_data_t));
        dlms_set_data(&dlms, mbus_list, mbus_count);

        size_t plaintext_size = decrypt_aes_gcm(meter_key, sizeof(meter_key), dlms.start_vector, sizeof(dlms.start_vector), dlms.apdu, dlms.apdu_length, plaintext_data, sizeof(plaintext_data));

        memset(&kaifa, 0, sizeof(kaifa_data_t));
        parse_obis_codes(&kaifa, plaintext_data, plaintext_size);


        if(kaifa_data_differnce(&kaifa, &kaifa_old, 0.05)){
            memcpy(&kaifa_old, &kaifa, sizeof(kaifa_data_t));
            xQueueSend(data_queue, &kaifa, 100/portTICK_PERIOD_MS);
        }

    }
}


void modem_thread(void * param){

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buffer[13] = {0};
    sprintf(buffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    

    sim7020e_init();
    sim7020e_handle_connection();
    mgudp_init(network_read_data_blocking_with_timeout, network_send_data);

    for(;;){
        if(!sim7020e_is_connected()){
            sim7020e_connect_udp(SERVER, PORT);
        }else{ // connected and ready to authenticate
            if(!mgudp_is_authenticated()){
                mgudp_authenticate(mac, secret_key);
            }else{ // Authenticated and ready to send data
                data_request_t request_data;
                if(xQueuePeek(request_queue, &request_data, 100/portTICK_PERIOD_MS)==pdTRUE){
                    uint8_t meter_key[16] = {0};
                    if(mgudp_request_and_receive_data_encrypted(request_data.request, request_data.size, meter_key, sizeof(meter_key))){
                        xQueueReceive(request_queue, &request_data, 0);
                        xQueueSend(meter_key_queue, meter_key, 100/portTICK_PERIOD_MS);
                    }
                }
                kaifa_data_t data = {0};
                if(xQueuePeek(data_queue, &data, 100/portTICK_PERIOD_MS)==pdTRUE){
                    
                    uint8_t kaifa_buffer[36] = {0};
                    kaifa_data_to_buffer(kaifa_buffer, 36, &data);
                    //ESP_LOG_BUFFER_HEX("USDP Data", kaifa_buffer, 36);

                    if(mgudp_send_data_encrypted(kaifa_buffer, sizeof(kaifa_buffer))){
                        xQueueReceive(data_queue, &data, 0);
                    }
                    
                }
            }
        }

    }

}

void generate_key(){
    nvs_handle_t nvs;
    nvs_flash_init();
    nvs_open("key_storage", NVS_READWRITE, &nvs);
    size_t key_len = 0;
    esp_err_t err = nvs_get_blob(nvs, "key", NULL, &key_len);
    if(err!=ESP_OK || key_len==0){
        uint8_t secret_key[16] = {0};
        esp_fill_random(secret_key, 16);
        ESP_LOG_BUFFER_HEX("KEY Gen", secret_key, 16);
        nvs_set_blob(nvs, "key", secret_key, 16);
        ESP_ERROR_CHECK(nvs_commit(nvs));
    }
    nvs_get_blob(nvs, "key", secret_key, &key_len);
    nvs_close(nvs);
    ESP_LOG_BUFFER_HEX("NVS", secret_key, key_len);
}



void generate_key_fuse(){
    //ESP_LOGI("HMAC", "Purpose: %d", esp_efuse_get_key_purpose(EFUSE_BLK_KEY5));

    uint8_t hmac_key[32] = {0};
    uint8_t comp[54] = {0};
    esp_efuse_read_block(EFUSE_BLK_KEY5, comp, 0, 64*8);
    //ESP_LOG_BUFFER_HEX("HMAC Key read", comp, 64);


    if(esp_efuse_block_is_empty(EFUSE_BLK_KEY5) && esp_efuse_key_block_unused(EFUSE_BLK_KEY5)){
        esp_fill_random(hmac_key, 32);
        //ESP_LOG_BUFFER_HEX("HMAC Key gen", hmac_key, 32);
        esp_efuse_write_key(EFUSE_BLK_KEY3, ESP_EFUSE_KEY_PURPOSE_HMAC_UP, hmac_key, sizeof(hmac_key));

        esp_efuse_read_block(EFUSE_BLK_KEY5, comp, 0, 32*8);
        //ESP_LOG_BUFFER_HEX("HMAC Key read", comp, 32);
    }
}


void app_main(void){ 


    // generate_key_fuse();
    
    generate_key();

    data_queue = xQueueCreate(100, sizeof(kaifa_data_t));
    request_queue = xQueueCreate(1, sizeof(data_request_t));
    meter_key_queue = xQueueCreate(1, 16);

    get_meter_key();

    xTaskCreate(mbus_thread, "mbus thread", 2048, NULL, 0, NULL);
    xTaskCreate(modem_thread, "modem thread", 4096, NULL, 0, NULL);

}
