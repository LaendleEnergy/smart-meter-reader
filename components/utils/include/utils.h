#ifndef UTILS
#define UTILS

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <mbedtls/gcm.h>


c
Copy code
/**
 * @file aes_gcm.h
 * @brief AES-GCM encryption and decryption functions.
 */

/**
 * @brief Encrypts plaintext using AES-GCM.
 *
 * @param key Pointer to the AES key.
 * @param key_len Length of the AES key.
 * @param iv Pointer to the initialization vector (IV).
 * @param iv_len Length of the IV.
 * @param plaintext Pointer to the plaintext.
 * @param plaintext_len Length of the plaintext.
 * @param cipher_text Pointer to the buffer for the ciphertext.
 * @param cipher_text_len Length of the ciphertext buffer.
 * @param tag Pointer to the buffer for the authentication tag.
 * @param tag_len Length of the authentication tag buffer.
 * @return Size of the encrypted ciphertext.
 */
size_t encrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * plaintext, size_t plaintext_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t *tag, size_t tag_len);

/**
 * @brief Decrypts ciphertext using AES-GCM.
 *
 * @param key Pointer to the AES key.
 * @param key_len Length of the AES key.
 * @param iv Pointer to the initialization vector (IV).
 * @param iv_len Length of the IV.
 * @param cipher_text Pointer to the ciphertext.
 * @param cipher_text_len Length of the ciphertext.
 * @param plaintext Pointer to the buffer for the decrypted plaintext.
 * @param plaintext_len Length of the plaintext buffer.
 * @return Size of the decrypted plaintext.
 */
size_t decrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t * plaintext, size_t plaintext_len);


#endif