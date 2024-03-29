#include "dlms.h"

int8_t dlms_set_data(dlms_data_t * dlms, mbus_packet_t * mbus_list, size_t mbus_packet_count){
    size_t current_apdu_length = 0;
    mbus_packet_t * mbus = mbus_list;
    for(uint8_t i = 0; i < mbus_packet_count; i++, mbus++){

        if(mbus->control_information==0x00){
            if(mbus->data_length>23){
                uint16_t offset = 0;
                dlms->cipher_service = *(mbus->data);
                offset = 1;
                dlms->system_title_length = *(mbus->data+offset);
                offset = 2;
                if(dlms->system_title_length>8){
                    dlms->system_title_length = 8;
                }
                memset(dlms->system_title, 0, dlms->system_title_length);
                memcpy(dlms->system_title, mbus->data+offset, dlms->system_title_length);
                offset+=dlms->system_title_length;

                if(*(mbus->data+offset)<=0x80){
                    dlms->apdu_length = *(mbus->data+offset);
                    offset+=1;
                }else if(*(mbus->data+offset)==0x82){
                    dlms->apdu_length = *(mbus->data+(offset+1)) << 8 | *(mbus->data+(offset+2));
                    offset+=3;
                }else if(*(mbus->data+offset)==0x81){
                    dlms->apdu_length = (*mbus->data+offset+1);
                    offset+=2;
                }
                dlms->security_control_byte=*(mbus->data+offset);
                offset+=1;
            
                dlms->frame_counter=*(mbus->data+offset)<<24 | *(mbus->data+offset+1)<<16 | *(mbus->data+offset+2)<<8 | *(mbus->data+offset+3);
                memcpy(dlms->start_vector, dlms->system_title, dlms->system_title_length); //Startvector = system title ++ frame counter
                memcpy(dlms->start_vector+dlms->system_title_length, mbus->data+offset, 4);
                offset+=4;
                if(sizeof(dlms->apdu)>=mbus->data_length){
                    memcpy(dlms->apdu, mbus->data+offset, mbus->data_length-offset);
                    current_apdu_length = mbus->data_length-offset;
                }

            }
        }else if(sizeof(dlms->apdu)>=current_apdu_length+mbus->data_length && current_apdu_length<dlms->apdu_length){
            memcpy(dlms->apdu+current_apdu_length, mbus->data, mbus->data_length);
            current_apdu_length = current_apdu_length+mbus->data_length;
        }
        
    }
    dlms->apdu_length = current_apdu_length;
    return 0;

}





