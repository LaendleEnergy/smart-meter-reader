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

void mbus_init();
size_t mbus_receive_multiple(mbus_packet_t * mbus_list, size_t count);
void mbus_parse_from_buffer(mbus_packet_t * mbus_packet, uint8_t * buffer);

#endif