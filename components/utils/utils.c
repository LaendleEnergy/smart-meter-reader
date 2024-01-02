#include "utils.h"

size_t encrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * plaintext, size_t plaintext_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t *tag, size_t tag_len){
    memset(tag, 0, tag_len);
    memset(cipher_text, 0, cipher_text_len);

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, key_len*8 );

    size_t output_size = 0;
    mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_ENCRYPT, iv, iv_len);
    mbedtls_gcm_update(&gcm, plaintext, plaintext_len, cipher_text, cipher_text_len, &output_size);
    mbedtls_gcm_finish(&gcm, NULL, 0, NULL, tag, tag_len);

    mbedtls_gcm_free(&gcm);

    return output_size;
}

size_t decrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t * plaintext, size_t plaintext_len){
    uint8_t tag[16] = {0};
    memset(plaintext, 0, plaintext_len);

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, key_len*8 );

    size_t output_size = 0;
    mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_DECRYPT, iv, iv_len);
    mbedtls_gcm_update(&gcm, cipher_text, cipher_text_len, plaintext, plaintext_len, &output_size);
    mbedtls_gcm_finish(&gcm, NULL, 0, NULL, tag, 16);

    mbedtls_gcm_free(&gcm);

    return output_size;
}
