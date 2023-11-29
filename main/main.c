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

// MBUS Defines
#define MBUS_UART UART_NUM_1
#define MBUS_RX 5
#define MBUS_BUFFER_SIZE 1024


#define MAX_FRAME_COUNT 2
#define KEY_SIZE 16
#define START_VECTOR_SIZE 12
#define PLAINTEXT_SIZE 512

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

uint8_t key[16] = {0x32, 0x69, 0x31, 0x63, 0x79, 0x79, 0x45, 0x6C, 0x59, 0x37, 0x34, 0x44, 0x73, 0x6D, 0x33, 0x75};

uint8_t udp_key[16] = {0xb2, 0x0d, 0x7a, 0x8c, 0x55, 0x1e, 0x3f, 0x9a,0x64, 0x0b, 0x2f, 0x78, 0xd1, 0x5a, 0x0c, 0x91};

mbus_packet_t mbus_list[MAX_FRAME_COUNT];
dlms_data_t dlms;
kaifa_data_t kaifa, kaifa_old;

// QueueHandle_t data_queue;

void to_hex_string(char * hex_buffer, uint16_t hex_buffer_size, uint8_t * buffer, uint16_t buffer_size){
    memset(hex_buffer, 0, hex_buffer_size);
    char * buf_ptr = hex_buffer;
    for(uint16_t i = 0; i < buffer_size && i< hex_buffer_size; i++){
        buf_ptr += sprintf(buf_ptr, "%02X", buffer[i]);
    }
}



uint32_t to_epoch(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second){
    uint32_t epoch = 0;


    return epoch;
}




//                 *** KUNDENSCHNITTSTELLE ***

// OBIS Code       Bezeichnung                      Wert
// 09.11.2023 08:45:40
// 0.0.96.1.0.255  Zaehlernummer:                   1KFM0200204032
// 0.0.42.0.0.255  COSEM printical device name:     KFM1200200204032
// 1.0.32.7.0.255  Spannung L1 (V):                 226.7
// 1.0.52.7.0.255  Spannung L2 (V):                 227.6
// 1.0.72.7.0.255  Spannung L3 (V):                 227.2
// 1.0.31.7.0.255  Strom L1 (A):                    0.0
// 1.0.51.7.0.255  Strom L2 (A):                    0.0
// 1.0.71.7.0.255  Strom L3 (A):                    0.0
// 1.0.1.7.0.255   Wirkleistung Bezug [kW]:         0.0
// 1.0.2.7.0.255   Wirkleistung Lieferung [kW]:     0.0
// 1.0.1.8.0.255   Wirkenergie Bezug [kWh]:         0.022
// 1.0.2.8.0.255   Wirkenergie Lieferung [kWh]:     0.0
// 1.0.3.8.0.255   Blindleistung Bezug [kW]:        0.007
// 1.0.4.8.0.255   Blindleistung Lieferung [kW]:    0.0                             



//Datatype=0x16 uint8, 0x12 uint16, 0x06 uint32, 0x00 None, 0x01 array + 0x03 quantity 3, 0x02 structure + 0x03 structure length 3, 
//0x03 boolean, 0x04 bitstring, 0x05 int32, 0x09 octet string, 0x10 int16, 0x11 uint8, 0x12 uint16, 0x13 compact array, 0x16 enum value

//0F0001A4640C       07E70B09 04 0C251E 00 FF C4 00       02              10          0906
// Start                 Timestamp            Timezone?      struct    struct quantity datatype 
//0000010000FF 09 0C 07E7.0B.09 04 0C:25:1E 00 FF C4 00  02020906 
//Timestamp          2023.11.09    12:25:30 

//0000600100FF 09 0E314B464D30323030        323034303332 
//02020906 00022A0000FF 09 104B464D31323030   323030 323034303332 

//02       03         09      06       0100200700FF    12     08EF     02       02      0F  FF  16   23   
//Struct length=3 octetstring length=6  Voltage L1   uint16  228.7    struct length=2  int8 -1 enum   V    

// 02     03         0906   0100340700FF 12 08F9     02020F FF 16 23 02030906  #   v l2
//struct length=3        Voltage L2                      /10   V

//0100480700FF 12 08F2     02020F FF 16 23 02030906  #   v l3
//                                /10    V 
//01001F0700FF 12 0000     02020F FE 16 21 02030906  !   c l1
//Current L1                     /100 

//0100330700FF 12 0000     02020F FE 16 21 02030906  !   c l2
//0100470700FF 12 0000     02020F FE 16 21 02030906  !   c l3

//0100010700FF 06 00000000 02020F 00 16 1B 02030906 
//                             /no scale

//0100020700FF 06 00000000 02020F 00 16 1B 02030906 >27

//0100010800FF 06 00000016 02020F 00 16 1E 02030906 >30
//0100020800FF 06 00000000 02020F 00 16 1E 02030906 >30

//0100030800FF 06 00000007 02020F 00 16 20  >32
//02     03    09    06   0100040800FF    06   00000000 02   02   0F  00  16  20 
//struct l=3  octet  l=6  zählerstandQ- uint32    0   struct l=2 int8  0 enum kWH 



bool kaifa_data_differnce(kaifa_data_t * kaifa, kaifa_data_t * kaifa_old, float offset){
    return abs(kaifa->voltage_l1-kaifa_old->voltage_l1)>kaifa->voltage_l1*offset || 
           abs(kaifa->voltage_l2-kaifa_old->voltage_l2)>kaifa->voltage_l2*offset ||
           abs(kaifa->voltage_l3-kaifa_old->voltage_l3)>kaifa->voltage_l3*offset || 
           abs(kaifa->current_l1-kaifa_old->current_l1)>kaifa->current_l1*offset ||
           abs(kaifa->current_l2-kaifa_old->current_l2)>kaifa->current_l2*offset ||
           abs(kaifa->current_l3-kaifa_old->current_l3)>kaifa->current_l3*offset ||
           abs(kaifa->active_power_plus-kaifa_old->active_power_plus)>kaifa->active_power_plus*offset ||
           abs(kaifa->active_power_minus-kaifa_old->active_power_minus)>kaifa->active_power_minus*offset ||
           abs(kaifa->reactive_power_plus-kaifa_old->reactive_power_plus)>kaifa->reactive_power_plus*offset ||
           abs(kaifa->reactive_power_minus-kaifa_old->reactive_power_minus)>kaifa->reactive_power_minus*offset ||
           abs(kaifa->active_energy_plus-kaifa_old->active_energy_plus)>kaifa->active_energy_plus*offset ||
           abs(kaifa->active_energy_minus-kaifa_old->active_energy_minus)>kaifa->active_energy_minus*offset;
}


void usdp_format_data(uint8_t * frame_buffer, size_t buf_size, kaifa_data_t * kaifa){
    if(buf_size<32) return;

    memset(frame_buffer, 0, buf_size);
    frame_buffer[0] = 0x0F;
    frame_buffer[1] = 0x00;

    frame_buffer[2] = kaifa->epoch >> 24 & 0xFF;
    frame_buffer[3] = kaifa->epoch >> 16 & 0xFF;
    frame_buffer[4] = kaifa->epoch >> 8 & 0xFF;
    frame_buffer[5] = kaifa->epoch & 0xFF;

    frame_buffer[6] = kaifa->scale_voltage;
    frame_buffer[7] = kaifa->voltage_l1 >> 8 & 0xFF;
    frame_buffer[8] = kaifa->voltage_l1 & 0xFF;
    frame_buffer[9] = kaifa->voltage_l2 >> 8 & 0xFF;
    frame_buffer[10] = kaifa->voltage_l2 & 0xFF;
    frame_buffer[11] = kaifa->voltage_l3 >> 8 & 0xFF;
    frame_buffer[12] = kaifa->voltage_l3 & 0xFF;

    frame_buffer[13] = kaifa->scale_current;
    frame_buffer[14] = kaifa->current_l1 >> 8 & 0xFF;
    frame_buffer[15] = kaifa->current_l1 & 0xFF;
    frame_buffer[16] = kaifa->current_l2 >> 8 & 0xFF;
    frame_buffer[17] = kaifa->current_l2 & 0xFF;
    frame_buffer[18] = kaifa->current_l3 >> 8 & 0xFF;
    frame_buffer[19] = kaifa->current_l3 & 0xFF;

    frame_buffer[20] = kaifa->scale_power;
    frame_buffer[21] = kaifa->active_power_plus >> 8 & 0xFF;
    frame_buffer[22] = kaifa->active_power_plus & 0xFF;
    frame_buffer[23] = kaifa->active_power_minus >> 8 & 0xFF;
    frame_buffer[24] = kaifa->active_power_minus & 0xFF;
    frame_buffer[25] = kaifa->reactive_power_plus >> 8 & 0xFF;
    frame_buffer[26] = kaifa->reactive_power_plus & 0xFF;
    frame_buffer[27] = kaifa->reactive_power_minus >> 8 & 0xFF;
    frame_buffer[28] = kaifa->reactive_power_minus & 0xFF;

    frame_buffer[29] = kaifa->scale_energy;
    frame_buffer[30] = kaifa->active_energy_plus >> 8 & 0xFF;
    frame_buffer[31] = kaifa->active_energy_minus & 0xFF;


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

        if(kaifa_data_differnce(&kaifa, &kaifa_old, 0.05)){
            memcpy(&kaifa_old, &kaifa, sizeof(kaifa_data_t));

            
        }


        
    }
}


// void challange_response(){
//     uint32_t rnd = esp_get_random();
// }

bool autheticated = false;
uint32_t frame_counter = 0;

typedef enum state {
    UNAUTHORIZED = 0x00,
    AUTH_STARTED = 0x01,
    AUTHORIZED = 0x02
}client_state_e;

client_state_e client_state = UNAUTHORIZED;


void compute_challenge_response(uint8_t * response, uint8_t * challenge, uint8_t * key){
    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);
    mbedtls_sha256_update(&sha256, (uint8_t*)challenge, 16);
    mbedtls_sha256_update(&sha256, key, 16);

    mbedtls_sha256_finish(&sha256, response);
}



bool usdp_authenticate(uint8_t * mac){
    uint8_t auth_frame[7] = {0xFF, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]};
    sim7020e_send_raw_data(auth_frame, 7);
    ESP_LOGI("USDP","Sending Authframe");

    while(network_has_data()<16){vTaskDelay(100/portTICK_PERIOD_MS);}

    uint8_t challenge[16] = {0};
    network_read_data(challenge, 16);
    ESP_LOGI("USDP","Challenge received");
    uint8_t response[33] = {0};
    response[0] = 0xFF;
    compute_challenge_response(response+1, challenge, udp_key);
    ESP_LOG_BUFFER_HEX("USDP", response, 33);
    sim7020e_send_raw_data(response, 33);

    while(network_has_data()<1){vTaskDelay(100/portTICK_PERIOD_MS);}
    
    uint8_t server_response;
    network_read_data(&server_response, 1);
    ESP_LOG_BUFFER_HEX("USDP", &server_response, 1);
    
    return server_response==AUTH_SUCCESS;
    
}

void modem_thread(void * param){

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char buffer[13] = {0};
    sprintf(buffer, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // sim7020e_set_data_received_callback(udp_data_received);
    sim7020e_init();

    if(sim7020e_connect_udp(SERVER, PORT)){
        sim7020e_handle_connection();
        while(!usdp_authenticate(mac)){
            ESP_LOGE("USDP","Authentication failed");
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
        ESP_LOGI("USDP","Authenticated");
        for(;;){
            kaifa_data_t data = {0};
            // if(xQueuePeek(data_queue, &data, 100/portTICK_PERIOD_MS)==pdTRUE){
            //     uint8_t frame_buffer[38] = {0};
            //     usdp_format_data(frame_buffer, 36, &data);
            //     sim7020e_send_raw_data(frame_buffer, 36);

            //     while(network_has_data()<=0){vTaskDelay(100/portTICK_PERIOD_MS);}
            //     uint8_t server_response;
            //     network_read_data(&server_response, 1);
            // }
        }
    }
    
}

void app_main(void){

    // data_queue = xQueueCreate(100, sizeof(kaifa_data_t));

    xTaskCreate(mbus_thread, "mbus thread", 2048, NULL, 0, NULL);
    xTaskCreate(modem_thread, "modem thread", 4096, NULL, 0, NULL);


    // for(;;){
    //     ESP_LOGI("Main Loop", "Running");
    //     vTaskDelay(1000);
    // }
}
