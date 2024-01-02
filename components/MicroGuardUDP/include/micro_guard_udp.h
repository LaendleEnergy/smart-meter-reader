#ifndef MGUDP
#define MGUDP

#include <stdbool.h>
#include <stdint.h>
#include <mbedtls/sha256.h>
#include <mbedtls/gcm.h>
#include <string.h>
#include <mbedtls/md.h>

#include <esp_log.h>
#include <esp_hmac.h>
#include <nvs.h>
#include <nvs_flash.h>
#include "utils.h"

#define AUTH_SUCCESS 0x22
#define AUTH_FAILED 0x11
#define DATA_SUCCESS 0x44
#define DATA_FAILED 0x33

#define AUTHENTICATED 1
#define NOT_AUTHENTICATED 0

typedef struct{
    uint8_t secret_key[32];
    uint8_t state;
    uint8_t start_vector[12];
    uint8_t session_key[16];
    uint64_t frame_counter;
    uint16_t frame_counter_offset;
    uint8_t challenge[16];
}mgudp_t;

void mgudp_init(int32_t (*read_callback)(uint8_t *, size_t, uint32_t), void (*send_callback)(uint8_t *, size_t));
bool mgudp_is_authenticated();
bool mgudp_authenticate(uint8_t * mac, uint8_t * secret_key);
bool mgudp_send_data_encrypted(uint8_t * data, size_t length);



#endif