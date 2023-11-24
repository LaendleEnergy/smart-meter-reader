#ifndef KAIFA
#define KAIFA

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <esp_log.h>

typedef struct{
    uint32_t epoch;
    uint32_t id;
    uint8_t scale_voltage;
    uint16_t voltage_l1;
    uint16_t voltage_l2;
    uint16_t voltage_l3;
    uint8_t scale_current;
    uint16_t current_l1;
    uint16_t current_l2;
    uint16_t current_l3;
    uint8_t scale_power;
    uint16_t active_power_plus;
    uint16_t active_power_minus;
    uint16_t reactive_power_plus;
    uint16_t reactive_power_minus;
    uint8_t scale_energy;
    uint16_t active_energy_plus;
    uint16_t active_energy_minus;
}kaifa_data_t;

void parse_obis_codes(kaifa_data_t * kaifa, uint8_t * data, size_t data_len);

#endif