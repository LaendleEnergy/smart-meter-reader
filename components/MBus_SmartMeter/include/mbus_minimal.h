#ifndef MBUS_MINIMAL
#define MBUS_MINIMAL

#include <driver/uart.h>
#include <string.h>
#include <esp_log.h>

#ifndef MBUS_UART
#define MBUS_UART UART_NUM_1
#endif
#ifndef MBUS_BUFFER_SIZE
#define MBUS_BUFFER_SIZE 1024
#endif

#ifndef MBUS_RX
#define MBUS_RX 5
#endif

typedef struct{
    uint8_t uart_num;
    uint8_t buffer[MBUS_BUFFER_SIZE];
}mbus_t;


typedef struct {
    uint8_t start;
    uint16_t length;
    uint8_t control;
    uint8_t address;
    uint8_t control_information;
    uint8_t source_address;
    uint8_t destination_address;
    uint8_t data[260];
    uint16_t data_length;
    uint8_t checksum;
    uint8_t checksum_calc;
    uint8_t stop;
    uint8_t fraq;
}mbus_packet_t;

/**
 * @brief Initializes the MBus communication handler.
 *
 * This function configures the UART parameters for MBus communication and initializes the necessary resources.
 */
void mbus_init();

/**
 * @brief Receives multiple MBus packets from the communication channel.
 *
 * This function receives multiple MBus packets, populating the provided array of MBus packet structures.
 * It stops receiving additional packets when it encounters a frame with control information 0x11 (secondary address).
 *
 * @param mbus_list Pointer to the array of MBus packet structures where the received data will be stored.
 * @param count Number of MBus packets to receive and store in the array.
 * @return The actual number of MBus packets received and stored in the array.
 */
size_t mbus_receive_multiple(mbus_packet_t * mbus_list, size_t count);

/**
 * @brief Parses an MBus packet from a provided byte buffer.
 *
 * This function extracts information from a byte buffer containing an MBus packet and populates the provided
 * MBus packet structure with the decoded information.
 *
 * @param mbus_packet Pointer to the MBus packet structure where the parsed data will be stored.
 * @param buffer Pointer to the byte buffer containing the MBus packet data.
 */
void mbus_parse_from_buffer(mbus_packet_t * mbus_packet, uint8_t * buffer);

#endif