#ifndef DLMS
#define DLMS

#include <string.h>
#include <mbedtls/gcm.h>
#include "mbus_minimal.h"

#define DLMS_APDU_SIZE 512

typedef struct{
    uint8_t cipher_service;
    uint8_t system_title_length;
    uint8_t system_title[8];
    uint16_t apdu_length;
    uint8_t security_control_byte;
    uint32_t frame_counter;
    uint8_t apdu[DLMS_APDU_SIZE];
    uint8_t start_vector[12];
}dlms_data_t;

int8_t dlms_set_data(dlms_data_t * dlms, mbus_packet_t * mbus_list, size_t mbus_packet_count);
size_t decrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t * plaintext, size_t plaintext_len);
size_t encrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * plaintext, size_t plaintext_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t *tag, size_t tag_len);
#endif