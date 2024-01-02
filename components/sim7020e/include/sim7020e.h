#ifndef SIM7020E
#define SIM7020E

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <driver/uart.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_log.h>

#define RX_BUFFER_SIZE 1520

#ifndef MODEM_BUFFER_SIZE
#define MODEM_BUFFER_SIZE 2048
#endif

#ifndef MODEM_SEND_BUFFER_SIZE
#define MODEM_SEND_BUFFER_SIZE 128
#endif

#ifndef MODEM_BAUDRATE
#define MODEM_BAUDRATE 115200
#endif

#ifndef MODEM_UART
#define MODEM_UART UART_NUM_0
#endif

#ifndef MODEM_RX
#define MODEM_RX 20
#endif

#ifndef MODEM_TX
#define MODEM_TX 21
#endif

#ifndef MODEM_RI
#define MODEM_RI 3
#endif

#ifndef MODEM_DTR
#define MODEM_DTR 7
#endif

#ifndef MODEM_PWRKEY
#define MODEM_PWRKEY 6
#endif

#ifndef MODEM_APN
#define MODEM_APN "iot.1nce.net"
#endif


#define TIMEOUT_1S 1000000
#define TIMEOUT_5S 5000000
#define TIMEOUT_10S 10000000

#define MY_DRV_EVENT_START 0x00
#define MY_DRV_EVENT_STOP 0x01

#define NETWORK_DATA_SIZE 1024

enum {
    TIMEOUT = -2,
    ERROR = -1
};

typedef struct {
    char address[64];
    char port[8];
}server_info_t;

// typedef struct {
//     uint8_t data[NETWORK_DATA_SIZE];
//     uint16_t available;
//     uint8_t * start;
//     uint8_t * end;
//     uint8_t * pos;
// }network_data_t;

typedef struct{
    uint8_t data[NETWORK_DATA_SIZE];
    size_t length;
}data_t;

enum {
    CLIENT_DISCONNECTED = 0,
    CLIENT_CONNECTED = 1
};


/**
 * @brief Initializes the SIM7020E module.
 */
void sim7020e_init();

/**
 * @brief Tests the connection to the SIM7020E module.
 */
void sim7020e_test_connection();

/**
 * @brief Manually configures the APN for the SIM7020E module.
 *
 * @return True if APN configuration is successful, false otherwise.
 */
bool sim7020e_apn_manual_config();

/**
 * @brief Performs a hard reset on the SIM7020E module.
 */
void sim7020e_hard_reset();

/**
 * @brief Sends a command to the SIM7020E module and waits for the response.
 *
 * @param cmd The command to be sent.
 * @param timeout_us Timeout duration in microseconds.
 * @param repeats Number of repeats.
 * @param response_count Number of expected responses.
 * @param ... Variable arguments for response pointers.
 * @return 0 if successful, -1 if an error occurs.
 */
int8_t sim7020e_send_command_and_wait_for_response(char * cmd, uint64_t timeout_us, uint8_t repeats, size_t response_count, ...);

/**
 * @brief Connects the SIM7020E module to a TCP server.
 */
void sim7020e_connect_tcp();

/**
 * @brief Gets the connection status of the SIM7020E module.
 *
 * @return Connection status code.
 */
int8_t sim7020e_get_connection_status();

/**
 * @brief Gets the network registration status of the SIM7020E module.
 *
 * @return Network registration status code.
 */
int8_t sim7020e_get_network_registration_status();

/**
 * @brief Handles the connection logic for the SIM7020E module.
 */
void sim7020e_handle_connection();

/**
 * @brief Checks if the SIM7020E module is currently connected.
 *
 * @return True if connected, false otherwise.
 */
bool sim7020e_is_connected();

/**
 * @brief Connects the SIM7020E module to a UDP server.
 *
 * @return True if UDP connection is successful, false otherwise.
 */
bool sim7020e_connect_udp();

/**
 * @brief Sets the callback function for data received events.
 *
 * @param callback Pointer to the callback function.
 */
void sim7020e_set_data_received_callback(void (*callback)(void * data, size_t len));

/**
 * @brief Sends raw data through the SIM7020E module.
 *
 * @param data Pointer to the data to be sent.
 * @param length Length of the data.
 */
void sim7020e_send_raw_data(void * data, size_t length);

/**
 * @brief Sends data through the network interface.
 *
 * @param data Pointer to the data to be sent.
 * @param length Length of the data.
 */
void network_send_data(uint8_t * data, size_t length);

/**
 * @brief Reads data from the network interface in a blocking manner.
 *
 * @param data Pointer to the buffer for storing the received data.
 * @param length Length of the data to be read.
 * @return Number of bytes read.
 */
size_t network_read_data_blocking(uint8_t * data, size_t length);

/**
 * @brief Reads data from the network interface with a specified timeout.
 *
 * @param data Pointer to the buffer for storing the received data.
 * @param length Length of the data to be read.
 * @param timeout Timeout duration in milliseconds.
 * @return Number of bytes read or -1 if a timeout occurs.
 */
int32_t network_read_data_blocking_with_timeout(uint8_t * data, size_t length, uint32_t timeout);


#endif
