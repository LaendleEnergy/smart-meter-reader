#ifndef UTILS
#define UTILS

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mbedtls/gcm.h>


size_t encrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * plaintext, size_t plaintext_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t *tag, size_t tag_len);
size_t decrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t * plaintext, size_t plaintext_len);


#endif