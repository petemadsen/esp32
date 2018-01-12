#ifndef AW_BT_RFCOMM_H
#define AW_BT_RFCOMM_H


typedef uint8_t* (*bt_rfcomm_parser)(uint8_t* data, uint16_t len, uint16_t* out_len);


void bt_rfcomm_init(bt_rfcomm_parser);


#endif
