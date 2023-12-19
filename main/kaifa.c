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
    uint8_t * start = find_in_mem(data, obis_timestamp, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+2;
        uint16_t year = *(start) << 8 | *(start+1);
        uint8_t month = *(start+2);
        uint8_t day = *(start+3);
        uint8_t hour = *(start+5);
        uint8_t minute = *(start+6);
        uint8_t second = *(start+7);

        kaifa->epoch = epoch_from_timestamp(year, month, day, hour, minute, second);

        //ESP_LOGI("KAIFA", "Timestamp: %02d.%02d.%d %02d:%02d:%02d", day, month, year, hour, minute, second);
    }

    uint8_t obis_meternumber[6] = {0x00, 0x00, 0x60, 0x01, 0x00, 0xff};
    start = find_in_mem(data, obis_meternumber, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint8_t meternumber_length = *(start);
        char meternumber[meternumber_length];
        memcpy(meternumber, start+1, meternumber_length);

        kaifa->id = get_hash(meternumber, meternumber_length);
        
        //ESP_LOGI("KAIFA", "Length: %d Meternumber: %s", meternumber_length, meternumber);
    }
    uint8_t obis_devicename[6] = {0x00, 0x00, 0x2A, 0x00, 0x00, 0xff};
    start = find_in_mem(data, obis_devicename, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size;
        uint8_t devicename_length = *(start);
        char devicename[devicename_length];
        memcpy(devicename, start+1, devicename_length);

        //ESP_LOGI("KAIFA", "Length: %d Devicename: %s", devicename_length, devicename);
    }
    uint8_t obis_voltage_l1[6] = {0x01, 0x00, 0x20, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_voltage_l1, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t voltage_l1 = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        //ESP_LOGI("KAIFA", "Voltage L1 %dV Scale: %d", voltage_l1, scale_factor);

        kaifa->voltage_l1 = voltage_l1;
        kaifa->scale_voltage = scale_factor;
    }
    uint8_t obis_voltage_l2[6] = {0x01, 0x00, 0x34, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_voltage_l2, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t voltage_l2 = *(start)<< 8 | *(start+1);
        // int8_t scale_factor = *(start+5);
        //ESP_LOGI("KAIFA", "Voltage L2 %dV Scale: %d", voltage_l2, scale_factor);

        kaifa->voltage_l2 = voltage_l2;
    }
    uint8_t obis_voltage_l3[6] = {0x01, 0x00, 0x48, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_voltage_l3, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t voltage_l3 = *(start)<< 8 | *(start+1);
        // int8_t scale_factor = *(start+5);
        //ESP_LOGI("KAIFA", "Voltage L3 %dV Scale: %d", voltage_l3, scale_factor);

        kaifa->voltage_l3 = voltage_l3;
    }
    uint8_t obis_current_l1[6] = {0x01, 0x00, 0x1F, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_current_l1, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t current_l1_buf = *(start)<< 8 | *(start+1);
        int8_t scale_factor = *(start+5);
        // float current_l1 = (float)current_l1_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Current L1 %fA", current_l1);

        kaifa->current_l1 = current_l1_buf;
        kaifa->scale_current = scale_factor;
    }
    uint8_t obis_current_l2[6] = {0x01, 0x00, 0x33, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_current_l2, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t current_l2_buf = *(start)<< 8 | *(start+1);
        // int8_t scale_factor = *(start+5);
        // float current_l2 = (float)current_l2_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Current L2 %fA", current_l2);

        kaifa->current_l2 = current_l2_buf;
    }
    uint8_t obis_current_l3[6] = {0x01, 0x00, 0x47, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_current_l3, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t current_l3_buf = *(start)<< 8 | *(start+1);
        // int8_t scale_factor = *(start+5);
        // float current_l3 = (float)current_l3_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Current L3 %fA", current_l3);

        kaifa->current_l3 = current_l3_buf;
    }
    uint8_t obis_active_power_plus[6] = {0x01, 0x00, 0x01, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_active_power_plus, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint32_t active_power_plus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        // float active_power_plus = (float)active_power_plus_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Active Power Plus %fkW", active_power_plus);

        kaifa->active_power_plus = active_power_plus_buf;
        kaifa->scale_power = scale_factor;
    }
    uint8_t obis_active_power_minus[6] = {0x01, 0x00, 0x02, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_active_power_minus, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint32_t active_power_minus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        // int8_t scale_factor = *(start+7);
        // float active_power_minus = (float)active_power_minus_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Active Power Minus %fkW", active_power_minus);

        kaifa->active_power_plus = active_power_minus_buf;
    }
    uint8_t obis_active_energy_plus[6] = {0x01, 0x00, 0x01, 0x08, 0x00, 0xff};
    start = find_in_mem(data, obis_active_energy_plus, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint32_t active_energy_plus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        int8_t scale_factor = *(start+7);
        // float active_energy_plus = (float)active_energy_plus_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Active Energy Plus %fkWh", active_energy_plus);

        kaifa->active_energy_plus = active_energy_plus_buf;
        kaifa->scale_energy = scale_factor;
    }
    uint8_t obis_active_energy_minus[6] = {0x01, 0x00, 0x02, 0x08, 0x00, 0xff};
    start = find_in_mem(data, obis_active_energy_minus, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint32_t active_energy_minus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        // int8_t scale_factor = *(start+7);
        // float active_energy_minus = (float)active_energy_minus_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Active Energy Minus %fkWh", active_energy_minus);

        kaifa->active_energy_minus = active_energy_minus_buf;
    }
    uint8_t obis_reactive_power_plus[6] = {0x01, 0x00, 0x01, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_reactive_power_plus, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t reactive_repower_plus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        // int8_t scale_factor = *(start+7);
        // float reactive_repower_plus = (float)reactive_repower_plus_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Reactive Power Plus %fkW", reactive_repower_plus);

        kaifa->reactive_power_plus = reactive_repower_plus_buf;
    }
    uint8_t obis_reactive_power_minus[6] = {0x01, 0x00, 0x02, 0x07, 0x00, 0xff};
    start = find_in_mem(data, obis_reactive_power_minus, data_len, obis_size);
    if(start!=NULL){
        start+=obis_size+1;
        uint16_t reactive_power_minus_buf = *(start)<< 24 | *(start+1)<<16 | *(start+2)<< 8 | *(start+3);
        // int8_t scale_factor = *(start+7);
        // float reactive_power_minus = (float)reactive_power_minus_buf/(float)scale_factor;
        //ESP_LOGI("KAIFA", "Reactive Power Minus %fkW", reactive_power_minus);

        kaifa->reactive_power_minus = reactive_power_minus_buf;
    }
}

bool kaifa_data_differnce(kaifa_data_t * kaifa, kaifa_data_t * kaifa_old, float offset){
    return abs(kaifa->voltage_l1-kaifa_old->voltage_l1)>kaifa->voltage_l1*offset || 
           abs(kaifa->voltage_l2-kaifa_old->voltage_l2)>kaifa->voltage_l2*offset ||
           abs(kaifa->voltage_l3-kaifa_old->voltage_l3)>kaifa->voltage_l3*offset || 
           abs(kaifa->current_l1-kaifa_old->current_l1)>kaifa->current_l1*offset ||
           abs(kaifa->current_l2-kaifa_old->current_l2)>kaifa->current_l2*offset ||
           abs(kaifa->current_l3-kaifa_old->current_l3)>kaifa->current_l3*offset ||
           kaifa->active_power_plus-kaifa_old->active_power_plus>kaifa->active_power_plus*offset ||
           kaifa->active_power_minus-kaifa_old->active_power_minus>kaifa->active_power_minus*offset ||
           abs(kaifa->reactive_power_plus-kaifa_old->reactive_power_plus)>kaifa->reactive_power_plus*offset ||
           abs(kaifa->reactive_power_minus-kaifa_old->reactive_power_minus)>kaifa->reactive_power_minus*offset ||
           kaifa->active_energy_plus-kaifa_old->active_energy_plus>kaifa->active_energy_plus*offset ||
           kaifa->active_energy_minus-kaifa_old->active_energy_minus>kaifa->active_energy_minus*offset;
}

void kaifa_data_to_json(kaifa_data_t * kaifa, char * json_buffer){
    sprintf(json_buffer, "{\"epoch\": %ld, \"voltageL1V\": %0.1f, \"voltageL2V\": %0.1f, \"voltageL3V\": %0.1f, \"currentL1A\": %0.2f, \"currentL2A\": %0.2f, \"currentL3A\": %0.2f, \"instantaneousActivePowerPlusW\": %0.3f, \"instantaneousActivePowerMinusW\": %0.3f, \"totalEnergyConsumedWh\": %0.3f, \"totalEnergyDeliveredWh\": %0.3f }",
                                            kaifa->epoch, 
                                            (float)kaifa->voltage_l1/10.0, (float)kaifa->voltage_l2/10.0, (float)kaifa->voltage_l3/10.0, 
                                            (float)kaifa->current_l1/100.0, (float)kaifa->current_l2/100.0, (float)kaifa->current_l3/100.0,
                                            (float)kaifa->active_power_plus/1000.0, (float)kaifa->active_power_minus/1000.0,
                                            (float)kaifa->active_energy_plus/1000.0,(float)kaifa->active_energy_minus/1000.0); 
}

void kaifa_data_to_buffer(uint8_t * buffer, size_t length, kaifa_data_t * kaifa){
    if(length<36) return;

    memset(buffer, 0, length);

    buffer[0] = kaifa->epoch >> 24 & 0xFF;
    buffer[1] = kaifa->epoch >> 16 & 0xFF;
    buffer[2] = kaifa->epoch >> 8 & 0xFF;
    buffer[3] = kaifa->epoch & 0xFF;

    buffer[4] = kaifa->scale_voltage;
    buffer[5] = kaifa->voltage_l1 >> 8 & 0xFF;
    buffer[6] = kaifa->voltage_l1 & 0xFF;
    buffer[7] = kaifa->voltage_l2 >> 8 & 0xFF;
    buffer[8] = kaifa->voltage_l2 & 0xFF;
    buffer[9] = kaifa->voltage_l3 >> 8 & 0xFF;
    buffer[10] = kaifa->voltage_l3 & 0xFF;

    buffer[11] = kaifa->scale_current;
    buffer[12] = kaifa->current_l1 >> 8 & 0xFF;
    buffer[13] = kaifa->current_l1 & 0xFF;
    buffer[14] = kaifa->current_l2 >> 8 & 0xFF;
    buffer[15] = kaifa->current_l2 & 0xFF;
    buffer[16] = kaifa->current_l3 >> 8 & 0xFF;
    buffer[17] = kaifa->current_l3 & 0xFF;

    buffer[18] = kaifa->scale_power;
    buffer[19] = kaifa->active_power_plus >> 24 & 0xFF;
    buffer[20] = kaifa->active_power_plus >> 16 & 0xFF;
    buffer[21] = kaifa->active_power_plus >> 8 & 0xFF;
    buffer[22] = kaifa->active_power_plus & 0xFF;

    buffer[23] = kaifa->active_power_minus >> 24 & 0xFF;
    buffer[24] = kaifa->active_power_minus >> 16 & 0xFF;
    buffer[25] = kaifa->active_power_minus >> 8 & 0xFF;
    buffer[26] = kaifa->active_power_minus & 0xFF;

    // buffer[23] = kaifa->reactive_power_plus >> 8 & 0xFF;
    // buffer[24] = kaifa->reactive_power_plus & 0xFF;
    // buffer[25] = kaifa->reactive_power_minus >> 8 & 0xFF;
    // buffer[26] = kaifa->reactive_power_minus & 0xFF;

    buffer[27] = kaifa->scale_energy;
    buffer[28] = kaifa->active_energy_plus >> 24 & 0xFF;
    buffer[29] = kaifa->active_energy_plus >> 16 & 0xFF;
    buffer[30] = kaifa->active_energy_plus >> 8 & 0xFF;
    buffer[31] = kaifa->active_energy_plus & 0xFF;
    buffer[32] = kaifa->active_energy_minus >> 24 & 0xFF;
    buffer[33] = kaifa->active_energy_minus >> 16 & 0xFF;
    buffer[34] = kaifa->active_energy_minus >> 8 & 0xFF;
    buffer[35] = kaifa->active_energy_minus & 0xFF;
}

void kaifa_generate_test_data(kaifa_data_t * kaifa){
    memset(kaifa, 0, sizeof(kaifa_data_t));
    kaifa->epoch = 1701339962;
    kaifa->scale_voltage = -1;
    kaifa->voltage_l1 = 2303;
    kaifa->voltage_l2 = 2305;
    kaifa->voltage_l3 = 2307;
    kaifa->scale_current = -2;
    kaifa->current_l1 = 345;
    kaifa->current_l2 = 456;
    kaifa->current_l3 = 567;
    kaifa->scale_power = -3;
    kaifa->active_power_plus = 1000;
    kaifa->active_power_minus = 1000;
    kaifa->reactive_power_plus = 1000;
    kaifa->reactive_power_minus = 1000;
    kaifa->scale_energy = -3;
    kaifa->active_energy_plus = 2000;
    kaifa->active_energy_minus = 1000;
    
}