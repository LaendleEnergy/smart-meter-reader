
#include "sim7020e.h"

char modem_receive_buffer[MODEM_BUFFER_SIZE] = {0};
char modem_send_buffer[MODEM_SEND_BUFFER_SIZE] = {0};

static server_info_t server_info = {0};

QueueHandle_t sendQueue;
data_t send_data_in, send_data_out;

static network_data_t network_data;

// static void (*data_received_callback)(void * data, size_t length);

void sim7020e_init(){

    gpio_reset_pin(MODEM_RI);
    gpio_set_direction(MODEM_RI, GPIO_MODE_INPUT);

    gpio_reset_pin(MODEM_DTR);
    gpio_set_direction(MODEM_DTR, GPIO_MODE_INPUT);

    gpio_reset_pin(MODEM_PWRKEY);
    gpio_set_direction(MODEM_PWRKEY, GPIO_MODE_OUTPUT);
    gpio_set_level(MODEM_PWRKEY, 0);
    

    uart_config_t uart_config = {
        .baud_rate = MODEM_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    // Configure UART parameters
    uart_param_config(MODEM_UART, &uart_config);
    uart_set_pin(MODEM_UART, MODEM_TX, MODEM_RX, -1, -1);
    uart_driver_install(MODEM_UART, MODEM_BUFFER_SIZE, 0, 0, NULL, 0);

    memset(modem_receive_buffer, 0, MODEM_BUFFER_SIZE);
    memset(modem_send_buffer, 0, MODEM_SEND_BUFFER_SIZE);

    memset(network_data.data, 0, NETWORK_DATA_SIZE);
    network_data.available = 0;
    network_data.start = network_data.data;
    network_data.end = network_data.data+NETWORK_DATA_SIZE;
    network_data.pos = network_data.data;

    sendQueue = xQueueCreate(1, sizeof(data_t));

    sim7020e_test_connection();

    //Reset default config
    sim7020e_send_command_and_wait_for_response("atz0", TIMEOUT_1S, 1, 1, "OK");
    //Disable cmd echo
    sim7020e_send_command_and_wait_for_response("ate0", TIMEOUT_1S, 1, 1, "OK");

    int8_t state = sim7020e_get_network_registration_status();
    if(state<=0){
        ESP_LOGI("MODEM ", "Not connected to Network");
        sim7020e_apn_manual_config();
        ESP_LOGI("MODEM ", "Network connection established");
    }else if(state==1){
        while(sim7020e_get_network_registration_status()==1);
        ESP_LOGI("MODEM ", "Network connection established");
    }
    
}

// void sim7020e_set_data_received_callback(void (*callback)(void * data, size_t len)){
//     data_received_callback = callback;
// }

void sim7020e_test_connection(){
    while(sim7020e_send_command_and_wait_for_response("AT", TIMEOUT_1S, 3, 1, "OK")==-2){
        sim7020e_hard_reset();
    }
}

void sim7020e_hard_reset(){
    ESP_LOGE("MODEM", "Rebooting");
    gpio_set_level(MODEM_PWRKEY, 1);
    gpio_set_level(MODEM_PWRKEY, 0);
    vTaskDelay(500/portTICK_PERIOD_MS);
    gpio_set_level(MODEM_PWRKEY, 1);
    vTaskDelay(2000/portTICK_PERIOD_MS);
    ESP_LOGE("MODEM", "Reboot done!");
}

int8_t sim7020e_get_network_registration_status(){
    // int8_t state =  sim7020e_send_command_and_wait_for_response("at+cgreg?", TIMEOUT_1S, 1, 24, 
    // "+CGREG: 0,0", "+CGREG: 0,3", "+CGREG: 0,4", "+CGREG: 0,6", "+CGREG: 0,7",
    // "+CGREG: 1,0", "+CGREG: 1,3", "+CGREG: 1,4", "+CGREG: 1,6", "+CGREG: 1,7",
    // "+CGREG: 2,0", "+CGREG: 2,3", "+CGREG: 2,4", "+CGREG: 2,6", "+CGREG: 2,7",
    // "+CGREG: 0,2", "+CGREG: 2,2", "+CGREG: 1,2", 
    // "+CGREG: 0,1", "+CGREG: 0,5", "+CGREG: 1,1", "+CGREG: 1,5", "+CGREG: 2,1", "+CGREG: 2,5");

    sim7020e_send_command_and_wait_for_response("at+cgreg?", TIMEOUT_1S, 1, 1, "OK");
    char * mode = strstr(modem_receive_buffer, "+CGREG: ")+strlen("+CGREG: ");
    if(mode==NULL) return -1;

    char * state_code = mode+2;

    if(*state_code=='0' || *state_code=='3' || *state_code=='4' || *state_code=='6' || *state_code=='7'){
        return 0;
    }else if(*state_code=='2'){
        return 1;
    }else if(*state_code=='5'){
        return 2;
    }else{
        return -1;
    }

}

bool sim7020e_apn_manual_config(){

    //disable RF
    sim7020e_send_command_and_wait_for_response("at+cfun=0", TIMEOUT_5S, 1, 1, "OK");
    
    memset(modem_send_buffer, 0, MODEM_SEND_BUFFER_SIZE);
    sprintf(modem_send_buffer, "AT*MCGDEFCONT=\"IP\",\"%s\"", MODEM_APN);
    sim7020e_send_command_and_wait_for_response(modem_send_buffer, TIMEOUT_10S, 1, 1, "OK");

    //enable RF
    sim7020e_send_command_and_wait_for_response("at+cfun=1", TIMEOUT_10S, 1, 1, "+CPIN: READY");

    uint64_t start_time = esp_timer_get_time();
    do{
        sim7020e_send_command_and_wait_for_response("at+cgreg=0", TIMEOUT_1S, 1, 1, "OK");
        if((esp_timer_get_time())-start_time>TIMEOUT_10S*10){
            return false;
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }while(sim7020e_get_network_registration_status()<2);

    sim7020e_send_command_and_wait_for_response("at+cgcontrdp=1", TIMEOUT_10S, 3, 1, "+CGCONTRDP:");
    // sim7020e_send_command_and_wait_for_response("", TIMEOUT_10S*5, 1, 1, "OK");

    sim7020e_send_command_and_wait_for_response("at+cfgri=1", TIMEOUT_5S, 1, 1, "OK");

    return true;    
}

int8_t sim7020e_get_connection_status(){
    return sim7020e_send_command_and_wait_for_response("at+cipstatus", TIMEOUT_5S, 1, 7, "CLOSED", "CLOSING", "REMOTE CLOSING", "INITIAL", "CONNECTING", "CONNECTED", "CONNECT OK");
}

void sim7020e_connect_tcp(char * server, char * port){

    strncpy(server_info.address, server, sizeof(server_info.address));
    strncpy(server_info.port, port, sizeof(server_info.port));

    sim7020e_send_command_and_wait_for_response("at+cipmux=0", TIMEOUT_1S, 1, 1, "OK");
    sim7020e_send_command_and_wait_for_response("at+cipmode=1", TIMEOUT_1S, 1, 1, "OK");

    memset(modem_send_buffer, 0, MODEM_SEND_BUFFER_SIZE);
    sprintf(modem_send_buffer, "at+cipstart=\"TCP\",\"%s\",\"%s\"", server_info.address, server_info.port);
    sim7020e_send_command_and_wait_for_response(modem_send_buffer, TIMEOUT_10S, 3, 1, "OK");

    if(sim7020e_send_command_and_wait_for_response("", TIMEOUT_10S*6, 1, 1, "CONNECT OK")==0){
        sim7020e_send_command_and_wait_for_response("at+cipchan", TIMEOUT_1S, 1, 1, "CONNECT");
    }

}

bool sim7020e_connect_udp(char * server, char * port){

    strncpy(server_info.address, server, sizeof(server_info.address));
    strncpy(server_info.port, port, sizeof(server_info.port));

    sim7020e_send_command_and_wait_for_response("at+cipmux=0", TIMEOUT_1S, 3, 1, "OK");
    sim7020e_send_command_and_wait_for_response("at+cipmode=1", TIMEOUT_1S, 3, 1, "OK");
    sim7020e_send_command_and_wait_for_response("at+cipclose=0", TIMEOUT_1S, 1, 1, "CLOSE OK");
    if(sim7020e_get_connection_status()>4){
        if(sim7020e_send_command_and_wait_for_response("at+cipchan", TIMEOUT_1S, 1, 1, "CONNECT")==0){
            return true;
        }
    }else{

        memset(modem_send_buffer, 0, MODEM_SEND_BUFFER_SIZE);
        sprintf(modem_send_buffer, "at+cipstart=\"UDP\",\"%s\",\"%s\"", server_info.address, server_info.port);
        if(sim7020e_send_command_and_wait_for_response(modem_send_buffer, TIMEOUT_10S, 3, 8, "CLOSED", "CLOSING", "REMOTE CLOSING", "INITIAL", "CONNECTING", "CONNECTED", "ALREADY CONNECT", "CONNECT OK")>4){
            for(uint8_t i = 0; i < 3; i++){
                if(sim7020e_send_command_and_wait_for_response("at+cipchan", TIMEOUT_1S, 1, 1, "CONNECT\r\n")==0){
                    return true;
                }
            }
        }
    }
    return false;
}

void network_add_data(uint8_t * data, size_t len){
    if(len<=NETWORK_DATA_SIZE-network_data.available){
        memcpy(network_data.start+network_data.available, data, len);
        network_data.available = network_data.available + len;
    }else{
        uint16_t offset = (len+network_data.available-NETWORK_DATA_SIZE);
        uint8_t * new_start_pos = network_data.start+offset;
        memmove(network_data.start, new_start_pos, network_data.available-offset);
        memcpy(network_data.start+network_data.available-offset, data, len);
        network_data.available = NETWORK_DATA_SIZE;
    }
}

void network_read_data(uint8_t * buffer, uint16_t len){
    if(len<=network_data.available){
        if(len>NETWORK_DATA_SIZE){
            len=NETWORK_DATA_SIZE;
        }
        memcpy(buffer, network_data.start, len);
        memmove(network_data.start, network_data.start+len, network_data.available-len);
    }
}

uint16_t network_has_data(){
    return network_data.available;
}

void handle_connection(){
    while(true){
        memset(&send_data_out, 0, sizeof(data_t));
        if(xQueueReceive(sendQueue, &send_data_out, 100/portTICK_PERIOD_MS)==pdTRUE){
            uart_write_bytes(MODEM_UART, send_data_out.data, send_data_out.length);
        }
        

        size_t buffered = 0;
        uart_get_buffered_data_len(MODEM_UART, &buffered);
        if(buffered>0){
            memset(modem_receive_buffer, 0, MODEM_BUFFER_SIZE);
            size_t len = uart_read_bytes(MODEM_UART, modem_receive_buffer, buffered < MODEM_BUFFER_SIZE ? buffered : MODEM_BUFFER_SIZE, 100/portTICK_PERIOD_MS);

            if(len>0){
                
                if(strstr(modem_receive_buffer, "CLOSED")!=NULL){
                    ESP_LOGI("MODEM RCV", "%s", modem_receive_buffer);
                    sim7020e_connect_tcp(server_info.address, server_info.port);
                }else{
                    ESP_LOG_BUFFER_HEX("MODEM RCV", modem_receive_buffer, len);
                    network_add_data((uint8_t*)modem_receive_buffer, len);
                }
            }
        }
        
    }
}

void sim7020e_handle_connection(){
    
    xTaskCreate(handle_connection, "modem_connection_handle", 1024, NULL, configMAX_PRIORITIES - 1, NULL);
    
}



void sim7020e_send_raw_data(void * data, size_t length){
    
    memset(&send_data_in, 0, sizeof(data_t));
    memcpy(send_data_in.data, data, length);
    send_data_in.length = length;

    xQueueSend(sendQueue, &send_data_in, 100/portTICK_PERIOD_MS);
}

int8_t sim7020e_send_command_and_wait_for_response(char * cmd, uint64_t timeout_us, uint8_t repeats, size_t response_count, ...){

    for(uint8_t i = 0; i < repeats; i++){

        // Copy command into the command buffer, add carrigde return and send command
        if(cmd!=modem_send_buffer){
            strncpy((char *)modem_send_buffer, cmd, MODEM_SEND_BUFFER_SIZE);
        }
        size_t cmd_len = strlen(cmd);
        modem_send_buffer[cmd_len] = '\r';
        modem_send_buffer[cmd_len+1] = '\0';

        uart_write_bytes(MODEM_UART, modem_send_buffer, cmd_len+1);
        ESP_LOGI("MODEM CMD", "%s", modem_send_buffer);

        // clear receive buffer and append incomming data, than check if one of the listed responses came back
        memset(modem_receive_buffer, 0, MODEM_BUFFER_SIZE);

        uint64_t diff = 0;
        char * ptr = modem_receive_buffer;
        uint64_t start_time = esp_timer_get_time();

        do{
            int char_count = uart_read_bytes(MODEM_UART, ptr, 6, 1);

            if(char_count>0){
                ptr+=char_count;

                va_list list;
                va_start(list, response_count);
                for(size_t i = 0; i < response_count; i++){
                    char current_resp[50] = {0};
                    strncpy(current_resp, va_arg(list, const char *), 50);
                    if(strstr(modem_receive_buffer, current_resp)!=NULL){
                        size_t len = 0;
                        while(ptr!=modem_receive_buffer+MODEM_BUFFER_SIZE && len>0){
                            ptr+=uart_read_bytes(MODEM_UART, ptr, 1, 1);
                        }

                        ESP_LOGI("MODEM RCD", "%s", modem_receive_buffer);
                        return i;
                    }
                }
                va_end(list);

                if(strstr(modem_receive_buffer, "ERROR")!=NULL){
                    ESP_LOGE("MODEM RCD", "%s", modem_receive_buffer);
                    return ERROR;
                }
            }

            
        diff = esp_timer_get_time()-start_time;
        }while(diff<timeout_us && ptr!=modem_receive_buffer+MODEM_BUFFER_SIZE);
    }
    ESP_LOGE("MODEM RCD", "%s", modem_receive_buffer);
    return TIMEOUT;
}