#ifndef KAIFA
#define KAIFA

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <stdbool.h>
// #include <freertos/queue.h>

typedef struct{
    uint32_t epoch;
    uint32_t id;
    int8_t scale_voltage;
    uint16_t voltage_l1;
    uint16_t voltage_l2;
    uint16_t voltage_l3;
    int8_t scale_current;
    uint16_t current_l1;
    uint16_t current_l2;
    uint16_t current_l3;
    int8_t scale_power;
    uint32_t active_power_plus;
    uint32_t active_power_minus;
    uint16_t reactive_power_plus;
    uint16_t reactive_power_minus;
    int8_t scale_energy;
    uint32_t active_energy_plus;
    uint32_t active_energy_minus;
}kaifa_data_t;

void parse_obis_codes(kaifa_data_t * kaifa, uint8_t * data, size_t data_len);
bool kaifa_data_differnce(kaifa_data_t * kaifa, kaifa_data_t * kaifa_old, float offset);
void kaifa_data_to_buffer(uint8_t * buffer, size_t length, kaifa_data_t * kaifa);
void kaifa_generate_test_data(kaifa_data_t * kaifa);
void kaifa_data_to_json(kaifa_data_t * kaifa, char * json_buffer);
#endif