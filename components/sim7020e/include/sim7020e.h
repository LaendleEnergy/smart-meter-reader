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

enum {
    TIMEOUT = -2,
    ERROR = -1
};

typedef struct {
    char address[64];
    char port[8];
}server_info_t;



void sim7020e_init();

void sim7020e_test_connection();
bool sim7020e_apn_manual_config();
void sim7020e_hard_reset();
int8_t sim7020e_send_command_and_wait_for_response(char * cmd, uint64_t timeout_us, uint8_t repeats, size_t response_count, ...);

void sim7020e_connect_tcp();
int8_t sim7020e_get_connection_status();
int8_t sim7020e_get_network_registration_status();
void sim7020e_handle_connection();

bool sim7020e_connect_udp();


void sim7020e_set_data_received_callback(void (*callback)(void * data, size_t len));
void sim7020e_send_raw_data(void * data, size_t length);
#endif


