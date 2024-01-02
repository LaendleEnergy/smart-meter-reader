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

/**
 * @brief Initializes the MGUDP communication handler.
 *
 * This function initializes the MGUDP communication handler by setting up callback functions for read and send operations.
 *
 * @param read_callback Pointer to the function for reading data with blocking and timeout.
 * @param send_callback Pointer to the function for sending data.
 */
void mgudp_init(int32_t (*read_callback)(uint8_t *, size_t, uint32_t), void (*send_callback)(uint8_t *, size_t));


/**
 * @brief Checks if MGUDP is currently authenticated.
 *
 * @return True if MGUDP is authenticated, false otherwise.
 */
bool mgudp_is_authenticated();

/**
 * @brief Authenticates with the MGUDP server.
 *
 * This function performs the authentication process with the MGUDP server using the provided MAC address and secret key.
 *
 * @param mac Pointer to the MAC address for authentication.
 * @param secret_key Pointer to the secret key for authentication.
 * @return True if authentication is successful, false otherwise.
 */
bool mgudp_authenticate(uint8_t * mac, uint8_t * secret_key);

/**
 * @brief Sends encrypted data using MGUDP.
 *
 * This function sends encrypted data using MGUDP if it is authenticated.
 *
 * @param data Pointer to the data to be sent.
 * @param data_length Length of the data to be sent.
 * @return True if the data is successfully sent, false otherwise.
 */
bool mgudp_send_data_encrypted(uint8_t * data, size_t length);



#endif