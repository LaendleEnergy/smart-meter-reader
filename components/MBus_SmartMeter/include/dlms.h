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

/**
 * @brief Sets DLMS data using a list of MBus packets
 *
 * This function takes a pointer to a DLMS data structure and a list of MBus
 * packets along with their count to set the DLMS data accordingly.
 *
 * @param dlms Pointer to the DLMS data structure to be modified.
 * @param mbus_list Pointer to the array of MBus packets.
 * @param mbus_packet_count Number of MBus packets in the array.
 *
 * @return An 8-bit signed integer indicating the status of the operation.
 *         0: Success
 *        -1: Invalid input parameters or failure during data setting.
 */

int8_t dlms_set_data(dlms_data_t * dlms, mbus_packet_t * mbus_list, size_t mbus_packet_count);
#endif