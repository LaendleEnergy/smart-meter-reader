#include "kaifa.h"

uint8_t * find_in_mem(uint8_t * haystack, uint8_t * needle, size_t haystack_size, size_t needle_size){
    for(size_t i = 0; i < haystack_size-needle_size; i++){
        size_t sum = 0;
        for(size_t j = 0; j<needle_size;j++){
            sum += *(haystack+i+j)==*(needle+j);
        }
        if(sum==needle_size){
            return haystack+i;
        }
    }
    return NULL;
}

uint32_t get_hash(char* s, size_t n) {
    int64_t p = 31, m = 1e9 + 7;
    int64_t hash = 0;
    int64_t p_pow = 1;
    for(int i = 0; i < n; i++) {
        hash = (hash + s[i] * p_pow) % m;
        p_pow = (p_pow * p) % m;
    }
    return hash;
}

uint32_t epoch_from_timestamp(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second){
    return second + minute*60 + hour*3600 + day*86400 +
    (year-70)*31536000 + ((year-69)/4)*86400 -
    ((year-1)/100)*86400 + ((year+299)/400)*86400;
}

void parse_obis_codes(kaifa_data_t * kaifa, uint8_t * data, size_t data_len){
    uint8_t obis_size = 6;

    uint8_t obis_timestamp[6] = {0x00, 0x00, 0x01, 0x00, 0x00, 0xff};
    uint8_t * start = find_in_mem(data, obis_timestamp, data_len, obis_size)+obis_size+2;
    if(start!=NULL){
        uint16_t year = *(start) << 8 | *(start+1);
        uint8_t month = *(start+2);
        uint8_t day = *(start+3);
        uint8_t hour = *(start+5);
        uint8_t minute = *(start+6);
        uint8_t second = *(start+7);

        kaifa->epoch = epoch_from_timestamp(year, month, day, hour, minute, second);

        printf("\n\nTimestamp: %02d.%02d.%d %02d:%02d:%02d\n", day, month, year, hour, minute, second);
    }

    uint8_t obis_meternumber[6] = {0x00, 0x00, 0x60, 0x01, 0x00, 0xff};
    start = find_in_mem(data, obis_meternumber, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint8_t meternumber_length = *(start);
        char meternumber[meternumber_length];
        memcpy(meternumber, start+1, meternumber_length);

        kaifa->id = get_hash(meternumber, meternumber_length);
        
        printf("Length: %d Meternumber: %s\n", meternumber_length, meternumber);
    }
    uint8_t obis_devicename[6] = {0x00, 0x00, 0x2A, 0x00, 0x00, 0xff};
    start = find_in_mem(data, obis_devicename, data_len, obis_size)+obis_size;
    if(start!=NULL){
        uint8_t devicename_length = *(start);
        char devicename[devicename_length];
        memcpy(devicename, start+1, devicename_length);

        printf("Length: %d Devicename: %s\n", devicename_length, devicename);
    }
    uint8_t obis_voltage_l1[6] = {0x01, 0x00, 0x20, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_voltage_l1, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t voltage_l1 = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        printf("Voltage L1 %dV Scale: %d\n", voltage_l1, scale_factor);

        kaifa->voltage_l1 = voltage_l1;
    }
    uint8_t obis_voltage_l2[6] = {0x01, 0x00, 0x34, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_voltage_l2, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t voltage_l2 = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        printf("Voltage L2 %dV Scale: %d\n", voltage_l2, scale_factor);

        kaifa->voltage_l2 = voltage_l2;
    }
    uint8_t obis_voltage_l3[6] = {0x01, 0x00, 0x48, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_voltage_l3, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t voltage_l3 = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        printf("Voltage L3 %dV Scale: %d\n", voltage_l3, scale_factor);

        kaifa->voltage_l3 = voltage_l3;
    }
    uint8_t obis_current_l1[6] = {0x01, 0x00, 0x1F, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_current_l1, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t current_l1_buf = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        float current_l1 = (float)current_l1_buf/(float)scale_factor;
        printf("Current L1 %fA\n", current_l1);

        kaifa->current_l1 = current_l1_buf;
    }
    uint8_t obis_current_l2[6] = {0x01, 0x00, 0x33, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_current_l2, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t current_l2_buf = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        float current_l2 = (float)current_l2_buf/(float)scale_factor;
        printf("Current L2 %fA\n", current_l2);

        kaifa->current_l2 = current_l2_buf;
    }
    uint8_t obis_current_l3[6] = {0x01, 0x00, 0x47, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_current_l3, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t current_l3_buf = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        float current_l3 = (float)current_l3_buf/(float)scale_factor;
        printf("Current L3 %fA\n", current_l3);

        kaifa->current_l3 = current_l3_buf;
    }
    uint8_t obis_active_power_plus[6] = {0x01, 0x00, 0x01, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_active_power_plus, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t active_power_plus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        float active_power_plus = (float)active_power_plus_buf/(float)scale_factor;
        printf("Active Power Plus %fkW\n", active_power_plus);

        kaifa->active_power_plus = active_power_plus_buf;
    }
    uint8_t obis_active_power_minus[6] = {0x01, 0x00, 0x02, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_active_power_minus, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t active_power_minus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        float active_power_minus = (float)active_power_minus_buf/(float)scale_factor;
        printf("Active Power Minus %fkW\n", active_power_minus);

        kaifa->active_power_plus = active_power_minus_buf;
    }
    uint8_t obis_active_energy_plus[6] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};
    start = find_in_mem(data, obis_active_energy_plus, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t active_energy_plus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        float active_energy_plus = (float)active_energy_plus_buf/(float)scale_factor;
        printf("Active Energy Plus %fkWh\n", active_energy_plus);

        kaifa->active_energy_plus = active_energy_plus_buf;
    }
    uint8_t obis_active_energy_minus[6] = {0x01, 0x00, 0x02, 0x08, 0x00, 0xff};
    start = find_in_mem(data, obis_active_energy_minus, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t active_energy_minus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        float active_energy_minus = (float)active_energy_minus_buf/(float)scale_factor;
        printf("Active Energy Minus %fkWh\n", active_energy_minus);

        kaifa->active_energy_minus = active_energy_minus_buf;
    }
    uint8_t obis_reactive_power_plus[6] = {0x01, 0x00, 0x01, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_reactive_power_plus, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t reactive_repower_plus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        float reactive_repower_plus = (float)reactive_repower_plus_buf/(float)scale_factor;
        printf("Reactive Power Plus %fkW\n", reactive_repower_plus);

        kaifa->reactive_power_plus = reactive_repower_plus_buf;
    }
    uint8_t obis_reactive_power_minus[6] = {0x01, 0x00, 0x02, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_reactive_power_minus, data_len, obis_size)+obis_size+1;
    if(start!=NULL){
        uint16_t reactive_power_minus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        float reactive_power_minus = (float)reactive_power_minus_buf/(float)scale_factor;
        printf("Reactive Power Minus %fkW\n", reactive_power_minus);

        kaifa->reactive_power_minus = reactive_power_minus_buf;
    }
}