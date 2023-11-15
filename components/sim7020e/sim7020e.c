
#include "sim7020e.h"

char modem_receive_buffer[MODEM_BUFFER_SIZE] = {0};
char modem_send_buffer[MODEM_SEND_BUFFER_SIZE] = {0};

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


    sim7020e_test_connection();

    sim7020e_apn_manual_config();

    sim7020e_connect_tcp();
}

void sim7020e_test_connection(){
    while(sim7020e_send_command_and_wait_for_response("AT", TIMEOUT_1S, 3, 1, "OK")!=0){
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

bool sim7020e_apn_manual_config(){
    // sim7020e_send_command_and_wait_for_response("at+creset", TIMEOUT_5S, 1, 0, "");
    sim7020e_send_command_and_wait_for_response("atz0", TIMEOUT_1S, 1, 1, "OK");
    sim7020e_send_command_and_wait_for_response("ate0", TIMEOUT_1S, 1, 1, "OK");

    //disable RF
    sim7020e_send_command_and_wait_for_response("at+cfun=0", TIMEOUT_1S, 1, 1, "OK");
    
    memset(modem_send_buffer, 0, MODEM_SEND_BUFFER_SIZE);
    sprintf(modem_send_buffer, "AT*MCGDEFCONT=\"IP\",\"%s\"", MODEM_APN);
    sim7020e_send_command_and_wait_for_response(modem_send_buffer, TIMEOUT_10S, 1, 1, "OK");

    //enable RF
    sim7020e_send_command_and_wait_for_response("at+cfun=1", TIMEOUT_10S, 1, 1, "+CPIN: READY");

    uint64_t start_time = esp_timer_get_time();
    do{
        sim7020e_send_command_and_wait_for_response("at+cgreg=1", TIMEOUT_1S, 1, 1, "OK");
        if((esp_timer_get_time())-start_time>TIMEOUT_10S*10){
            return false;
        }
    }while(sim7020e_send_command_and_wait_for_response("at+cgreg?", TIMEOUT_1S, 1, 1, "+CGREG: 1,5", "+CGREG: 1,0")<0);

    sim7020e_send_command_and_wait_for_response("at+cgcontrdp=1", TIMEOUT_10S, 1, 1, "+CGCONTRDP:");
    sim7020e_send_command_and_wait_for_response("", TIMEOUT_10S, 1, 1, "OK");

    sim7020e_send_command_and_wait_for_response("at+cfgri=1", TIMEOUT_5S, 1, 1, "OK");

    return true;    
}

void sim7020e_connect_tcp(){

    sim7020e_send_command_and_wait_for_response("at+cipmux=0", TIMEOUT_1S, 1, 1, "OK");
    sim7020e_send_command_and_wait_for_response("at+cipmode=1", TIMEOUT_1S, 1, 1, "OK");

    memset(modem_send_buffer, 0, MODEM_SEND_BUFFER_SIZE);
    sprintf(modem_send_buffer, "at+cipstart=\"TCP\",\"45.145.224.10\",\"1884\"");;
    sim7020e_send_command_and_wait_for_response(modem_send_buffer, TIMEOUT_10S, 1, 1, "CONNECT OK");

    sim7020e_send_command_and_wait_for_response("at+cipchan", TIMEOUT_1S, 1, 1, "CONNECT");

    while(true){
        memset(modem_receive_buffer, 0, MODEM_BUFFER_SIZE);
        size_t len = uart_read_bytes(MODEM_UART, modem_receive_buffer, 10, 1);
        if(len>0)
            ESP_LOGI("MODEM RCV", "%s", modem_receive_buffer);
    }
}

void sim7020e_send_raw_data(uint8_t * data, size_t length){
    uart_write_bytes(MODEM_UART, data, length);
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