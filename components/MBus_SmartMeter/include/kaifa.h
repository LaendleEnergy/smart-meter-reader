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

/**
 * @brief Parses OBIS codes from a data buffer and populates a Kaifa data structure.
 *
 * This function extracts relevant information, such as timestamp, meter number, voltages, currents,
 * and power values, from a data buffer based on predefined OBIS codes. It populates the provided Kaifa
 * data structure with the extracted information.
 *
 * @param kaifa Pointer to the Kaifa data structure to be populated.
 * @param data Pointer to the data buffer containing OBIS codes and values.
 * @param data_len Length of the data buffer.
 */
void parse_obis_codes(kaifa_data_t * kaifa, uint8_t * data, size_t data_len);

/**
 * @brief Compares two Kaifa data structures and determines if they differ beyond a specified offset.
 *
 * This function compares two Kaifa data structures and returns true if their differences exceed a specified
 * offset for any of the monitored parameters (voltage, current, power, energy).
 *
 * @param kaifa Pointer to the new Kaifa data structure.
 * @param kaifa_old Pointer to the old Kaifa data structure for comparison.
 * @param offset The allowable difference offset as a fraction.
 * @return True if the differences exceed the specified offset, false otherwise.
 */
bool kaifa_data_differnce(kaifa_data_t * kaifa, kaifa_data_t * kaifa_old, float offset);

/**
 * @brief Converts Kaifa data to a byte buffer for transmission.
 *
 * This function converts the data stored in a Kaifa data structure to a byte buffer suitable for transmission
 * and stores it in the provided buffer.
 *
 * @param buffer Pointer to the buffer for storing the converted data.
 * @param length Size of the buffer.
 * @param kaifa Pointer to the Kaifa data structure.
 */
void kaifa_data_to_buffer(uint8_t * buffer, size_t length, kaifa_data_t * kaifa);

/**
 * @brief Generates test data for a Kaifa data structure.
 *
 * This function populates a Kaifa data structure with sample data for testing purposes.
 *
 * @param kaifa Pointer to the Kaifa data structure to be populated with test data.
 */
void kaifa_generate_test_data(kaifa_data_t * kaifa);

/**
 * @brief Converts Kaifa data to a JSON-formatted string.
 *
 * This function converts the data stored in a Kaifa data structure to a JSON-formatted string
 * and stores it in the provided buffer.
 *
 * @param kaifa Pointer to the Kaifa data structure.
 * @param json_buffer Pointer to the buffer for storing the JSON-formatted string.
 */
void kaifa_data_to_json(kaifa_data_t * kaifa, char * json_buffer);
#endif