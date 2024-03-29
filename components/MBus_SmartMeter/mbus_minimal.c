#include "mbus_minimal.h"

static mbus_t mbus;

void mbus_init(){
    uart_config_t uart_config = {
        .baud_rate = 2400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    // Configure UART parameters
    uart_param_config(MBUS_UART, &uart_config);
    uart_set_pin(MBUS_UART, -1, MBUS_RX, -1, -1);
    uart_driver_install(MBUS_UART, MBUS_BUFFER_SIZE, 0, 0, NULL, 0);

    mbus.uart_num = MBUS_UART;
    memset(mbus.buffer, 0, MBUS_BUFFER_SIZE);

}

uint8_t mbus_calc_checksum(mbus_packet_t * mbus_packet){

    uint32_t checksum;
    checksum = mbus_packet->control + mbus_packet->address + mbus_packet->control_information + mbus_packet->destination_address + mbus_packet->source_address;
    for(uint8_t i = 0; i < mbus_packet->data_length; i++){
        checksum+=mbus_packet->data[i];
    }

    return (uint8_t)checksum&0x000000ff;

}

int16_t mbus_receive(mbus_packet_t * mbus_packet){
    uint8_t start_buffer = 0;
    memset(mbus.buffer, 0, MBUS_BUFFER_SIZE);
    do{

        uart_read_bytes(MBUS_UART, &start_buffer, 1, 100);
        mbus.buffer[3] = mbus.buffer[2];
        mbus.buffer[2] = mbus.buffer[1];
        mbus.buffer[1] = mbus.buffer[0];
        mbus.buffer[0] = start_buffer;

    }while(!(mbus.buffer[0]==0x68 && mbus.buffer[3] == 0x68 && mbus.buffer[1] == mbus.buffer[2]));

    mbus_packet->start = mbus.buffer[0];
    mbus_packet->length = mbus.buffer[1];

    if(mbus_packet->length>MBUS_BUFFER_SIZE){
        return -1;
    }
    
    uart_read_bytes(MBUS_UART, &mbus.buffer[4], mbus_packet->length+2, 2000/portTICK_PERIOD_MS);

    mbus_packet->control = mbus.buffer[4];
    mbus_packet->address = mbus.buffer[5];
    mbus_packet->control_information = mbus.buffer[6];
    mbus_packet->source_address = mbus.buffer[7];
    mbus_packet->destination_address = mbus.buffer[8];
    mbus_packet->data_length = mbus_packet->length-5;
    memcpy(mbus_packet->data, &mbus.buffer[9], mbus_packet->data_length);
    mbus_packet->checksum = mbus.buffer[9+mbus_packet->data_length];
    mbus_packet->stop = mbus.buffer[10+mbus_packet->data_length];

    mbus_packet->checksum_calc = mbus_calc_checksum(mbus_packet);

    if(mbus_packet->stop==0x16){
        //ESP_LOGI("MBUS", "Complete Frame read");
    }else{
        //ESP_LOGE("MBUS", "Stop is not 0x16!");
    }

    if(mbus_packet->checksum_calc==mbus_packet->checksum){
        //ESP_LOGI("MBUS", "Checksum OK");
    }else{
        //ESP_LOGE("MBUS", "Checksum failed");
    }
    
    // mbus_packet->fraq = mbus_packet->control_information==0x00;

    if(mbus_packet->control_information==0x11){
        uart_flush_input(MBUS_UART);
    }

    return mbus_packet->length+1+1+1+1+1+1;  //1Byte start+ 1Byte length+ 1Byte length+ 1Byte start+ 1Byte checksum+ 1Byte end

}

void mbus_parse_from_buffer(mbus_packet_t * mbus_packet, uint8_t * buffer){
    mbus_packet->start = buffer[0];
    mbus_packet->length = buffer[1];
    mbus_packet->control = buffer[4];
    mbus_packet->address = buffer[5];
    mbus_packet->control_information = buffer[6];
    mbus_packet->source_address = buffer[7];
    mbus_packet->destination_address = buffer[8];
    mbus_packet->data_length = mbus_packet->length-5;
    memcpy(mbus_packet->data, &buffer[9], mbus_packet->data_length);
    mbus_packet->checksum = buffer[4+mbus_packet->length+1];
    mbus_packet->stop = buffer[4+mbus_packet->length+2];
}

size_t mbus_receive_multiple(mbus_packet_t * mbus_list, size_t count){
    memset(&mbus_list[0], 0, sizeof(mbus_packet_t));
    
    size_t counter = 1;
    mbus_receive(&mbus_list[0]);
    if(mbus_list[0].control_information==0x00){
        for(uint8_t i = 1; i < count && mbus_list[i-1].control_information!=0x11; i++){
            memset(&mbus_list[i], 0, sizeof(mbus_packet_t));
            mbus_receive(&mbus_list[i]);
            counter++;
        }
    }

    return counter;
}
