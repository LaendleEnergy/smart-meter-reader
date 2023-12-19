#include "usdp.h"

static usdp_t usdp;
static int32_t (*read_data_blocking_with_timeout)(uint8_t *data, size_t length, uint32_t timeout);
static void (*send_data)(uint8_t *data, size_t length);

void usdp_init(int32_t (*read_callback)(uint8_t *, size_t, uint32_t), void (*send_callback)(uint8_t *, size_t)){
    memset(&usdp, 0, sizeof(usdp_t));
    read_data_blocking_with_timeout = read_callback;
    send_data = send_callback;
}

void compute_challenge_response(uint8_t * response, uint8_t * challenge, uint8_t * key){
    mbedtls_sha256_context sha256;
    mbedtls_sha256_init(&sha256);
    mbedtls_sha256_starts(&sha256, 0);
    mbedtls_sha256_update(&sha256, challenge, 16);
    mbedtls_sha256_update(&sha256, key, 16);

    mbedtls_sha256_finish(&sha256, response);
}

void derive_key(uint8_t * derived_key, uint8_t * secret_key, uint8_t * challenge){
    mbedtls_md_context_t md;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&md);

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md_type);
    mbedtls_md_setup(&md, md_info, 1);

    mbedtls_md_hmac_starts(&md, secret_key, 16);
    mbedtls_md_hmac_update(&md, challenge, 16);

    mbedtls_md_hmac_finish(&md, derived_key);
}

int8_t check_response(uint8_t * response, uint8_t * session_key, uint64_t frame_counter){

    mbedtls_md_context_t md;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&md);

    uint8_t buffer_OK[9] = {0};
    memcpy(buffer_OK, &frame_counter, 8);
    buffer_OK[8] = DATA_SUCCESS;

    uint8_t buffer_ERR[9] = {0};
    memcpy(buffer_ERR, &frame_counter, 8);
    buffer_ERR[8] = DATA_FAILED;


    uint8_t response_comp_OK[32] = {0};
    uint8_t response_comp_ERR[32] = {0};

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md_type);
    mbedtls_md_setup(&md, md_info, 1);

    mbedtls_md_hmac_starts(&md, session_key, 16);
    mbedtls_md_hmac_update(&md, buffer_OK, 9);
    mbedtls_md_hmac_finish(&md, response_comp_OK);

    mbedtls_md_hmac_starts(&md, session_key, 16);
    mbedtls_md_hmac_update(&md, buffer_ERR, 9);
    mbedtls_md_hmac_finish(&md, response_comp_ERR);

    if(memcmp(response, response_comp_OK, 4)==0){
        return 1;
    }else if(memcmp(response, response_comp_ERR, 4)==0){
        return 0;
    }
    return -1;

}

bool usdp_is_authenticated(){
    return usdp.state;
}

bool usdp_authenticate(uint8_t * mac, uint8_t * secret_key){
    uint8_t auth_frame[7] = {0xFF, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]};

    for(uint8_t i = 0; i < 5; i++){
        send_data(auth_frame, 7);
        ESP_LOGI("USDP","Sending Authframe");

        memset(usdp.challenge, 0, 16);
        // network_read_data_blocking(challenge, 16);
        if(read_data_blocking_with_timeout(usdp.challenge, 16, 3000)<0){
            ESP_LOGE("USDP","Receive Challenge timeout");
            continue;
        }

        ESP_LOGI("USDP","Challenge received");

        uint8_t response[33] = {0};
        response[0] = 0xFF;

        // uint8_t tag[16] = {0};
        // encrypt_aes_gcm(secret_key, 16, start_vector, 12, usdp.challenge, 16, usdp.session_key, 16, tag, 0);
        uint8_t session_key[32] = {0};
        // esp_hmac_calculate(HMAC_KEY4, usdp.challenge, 16, session_key);

        derive_key(session_key, secret_key, usdp.challenge);
        ESP_LOG_BUFFER_HEX("HMAC KEY", session_key, 16);
        memcpy(usdp.session_key, session_key, 16);


        // derive_key(key, 16, challenge, 16);
        compute_challenge_response(response+1, usdp.challenge, usdp.session_key);
        ESP_LOG_BUFFER_HEX("USDP Challenge response", response, 33);
        send_data(response, 33);

        uint8_t server_response = 0;
        // network_read_data_blocking(&server_response, 1);

        if(read_data_blocking_with_timeout(&server_response, 1, 3000)<0){
            ESP_LOGE("USDP","Server response timeout");
            continue;
        }
        ESP_LOGI("USDP Server Response", "0x%02X",server_response);

        if(server_response==AUTH_FAILED){
            ESP_LOGE("USDP","Authentication failed");
            continue;
        }

        usdp.state = AUTHENTICATED;

        usdp.frame_counter = (uint64_t)usdp.challenge[11]<<56 | (uint64_t)usdp.challenge[10]<<48 | (uint64_t)usdp.challenge[9]<<40 | (uint64_t)usdp.challenge[8]<<32 | (uint64_t)usdp.challenge[7]<<24 | (uint64_t)usdp.challenge[6]<<16 | (uint64_t)usdp.challenge[5]<<8 | usdp.challenge[4];
        usdp.frame_counter_offset = usdp.frame_counter % 100;

        return usdp.state;
    }

    usdp.state = NOT_AUTHENTICATED;
    return usdp.state;
    
}

bool usdp_send_data_encrypted(uint8_t * data, size_t data_length){
    if(usdp.state == AUTHENTICATED){
        usdp.frame_counter+=usdp.frame_counter_offset;
        uint8_t usdp_iv[12] = {0};
        memcpy(usdp_iv, &usdp.frame_counter, 8);
        memcpy(usdp_iv+8, usdp.challenge, 4);
        
        // ESP_LOGI("USDP Frame Counter", "%lld", usdp.frame_counter);
        // ESP_LOG_BUFFER_HEX("USDP IV", usdp_iv, 12);
        // ESP_LOG_BUFFER_HEX("USDP key", usdp.session_key, 16);


        uint8_t encrypted_data[36] = {0};
        uint8_t tag[16] = {0};
        uint8_t usdp_frame[42] = {0};
        size_t length = encrypt_aes_gcm(usdp.session_key, sizeof(usdp.session_key), usdp_iv, sizeof(usdp_iv), data, data_length, encrypted_data, sizeof(encrypted_data), tag, sizeof(tag));
        ESP_LOG_BUFFER_HEX("USDP TAG", tag, 16);
        usdp_frame[0] = 0x0F; //Frame Type: Data
        usdp_frame[1] = 0;    //protocol Version: 0
        memcpy(usdp_frame+2, encrypted_data, length); //kaifa data
        memcpy(usdp_frame+2+length, tag, 4);          // 4 byte tag for validation;
        ESP_LOG_BUFFER_HEX("USDP", usdp_frame, 42);

        uint8_t server_response[4] = {0};
        uint8_t counter = 0;
        while(true){
            ESP_LOGI("USDP", "Sending data");
            send_data(usdp_frame, sizeof(usdp_frame));
            if(read_data_blocking_with_timeout(server_response, 4, 10000)<0){
                ESP_LOGE("USDP", "Response timeout");
            }
        	int8_t resp_stat = check_response(server_response, usdp.session_key, usdp.frame_counter);
            if(resp_stat==1){
                break;
            }else if(resp_stat == -1){
                usdp.state = NOT_AUTHENTICATED;
                return false;
            }

            counter++;
            if(counter>=3){
                usdp.state = NOT_AUTHENTICATED;
                return false;
            }
        };

        return true;
    }
    usdp.state = NOT_AUTHENTICATED;
    return false;
}

size_t encrypt_aes_gcm(uint8_t * key, uint8_t key_len, uint8_t * iv, uint8_t iv_len, uint8_t * plaintext, size_t plaintext_len, uint8_t * cipher_text, size_t cipher_text_len, uint8_t *tag, size_t tag_len){
    memset(tag, 0, tag_len);
    memset(cipher_text, 0, cipher_text_len);

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);
    mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, key_len*8 );

    size_t output_size = 0;
    mbedtls_gcm_starts(&gcm, MBEDTLS_GCM_ENCRYPT, iv, iv_len);
    mbedtls_gcm_update(&gcm, plaintext, plaintext_len, cipher_text, cipher_text_len, &output_size);
    mbedtls_gcm_finish(&gcm, NULL, 0, NULL, tag, tag_len);

    mbedtls_gcm_free(&gcm);

    return output_size;
}