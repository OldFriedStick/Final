/**
 * @file send.h
 * @author DGYYY (smiledgy@whu.edu.cn)
 * @brief head file of sending function
 * @version 1.6
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "vs_protocol.h"
#include "vpi_sensor_def.h"
#include "vpi_sensor_protocol.h"
#include "vpi_ringbuffer.h"

#ifndef _SEND_H_
#define _SEND_H_

#define MEG_SIZE 8
#define BUFFER_SIZE (10*(MEG_SIZE+2))

#define VS_CUSTOM_SPO2_MSG (VS_CUSTOM_MSG_START + 1)
#define VS_CUSTOM_ACK_MSG (VS_CUSTOM_MSG_START+2)

/*Other task use send_data_req fuction to send data by BLE,and pass
the data structure below as parameter.*/
typedef struct _SendParam {
    uint16_t size; // Data size
    uint8_t *data; // Pointer of data source buffer
} SendParam;

typedef struct _Spo2Msg {
    uint16_t id;
    uint16_t spo2;
    uint16_t loss_rate;
    uint16_t error_rate;
    uint16_t lead_off;
} __attribute((__packed__)) Spo2Msg;

typedef struct _AckMessage{
    uint16_t flag;
    uint16_t id;
}__attribute((__packed__))AckMessage;

int sending_task_init(void);

int read_res_data(int size, uint8_t *data);

void sending_task(void *param);

int sending_handler(void *cobj, uint32_t event_id, void *data);

int ble_handler(uint8_t id, uint8_t *data);

long send_res_data_req(SendParam *param);

uint8_t get_ble_state();
#endif
