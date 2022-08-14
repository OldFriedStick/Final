/**
 * @file send.c
 * @author DGYYY (smiledgy@whu.edu.cn)
 * @brief ble function file
 * @version 1.5
 * @date 2022-07-28
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "send.h"
#include "vpi_ringbuffer.h"
#include "osal_heap_api.h"
#include "osal_task_api.h"
#include "vpi_sensor_def.h"
#include "vpi_event_def.h"
#include "vpi_fifo.h"
#include "common_message_def.h"
#include "vpi_error_code.h"
#include "uart_printf.h"
#include "sensor_service.h"
#include "vpi_event.h"
#include "hal_rtc.h"
#include "osal_semaphore.h"
#include "sys_common.h"
#include "vpi_sw_timer.h"
#include "hal_sensor_common.h"
#include "osal_lock_api.h"

static RingBufCtrl *res_data_ctrl = NULL;
static uint8_t *res_data_buf      = NULL;

static RingBufCtrl *res_fifo_ctrl = NULL;
static uint8_t *res_fifo_buf      = NULL;

OsalSemaphore res_sem;
OsalMutex res_mutex;
static uint8_t current_ble_queue;
static uint8_t ble_state = BLE_STATE_DISCONNECTED;

static uint16_t res_trans_id = 0;

uint8_t get_ble_state(){
    return ble_state;
}

int sending_task_init(void)
{

    res_data_buf = (uint8_t *)osal_malloc(BUFFER_SIZE);
    osal_memset(res_data_buf, 0, BUFFER_SIZE);
    res_data_ctrl = vpi_ringbuf_init(res_data_buf, BUFFER_SIZE, false);

    if (res_data_ctrl == NULL || res_data_buf == NULL) {
        return VPI_ERR_NOMEM;
    }

    res_fifo_buf = (uint8_t *)osal_malloc(sizeof(BleTransmitData) * 10);
    osal_memset(res_fifo_buf, 0, sizeof(BleTransmitData) * 10);
    res_fifo_ctrl =
        vpi_ringbuf_init(res_fifo_buf, sizeof(BleTransmitData) * 10, false);

    if (res_fifo_ctrl == NULL || res_fifo_buf == NULL) {
        uart_printf("2");
        return VPI_ERR_NOMEM;
    }

    osal_init_mutex(&res_mutex);
    osal_init_sem(&res_sem);

    return VPI_SUCCESS;
}

long send_res_data_req(SendParam *param)
{
    vpi_event_notify(EVENT_BLE_TRAN_REQ, param);
    osal_sem_wait(&res_sem, 400);
    return 0;
}

int read_res_data(int size, uint8_t *data)
{
    if (data == NULL) {
        return VPI_ERR_INVALID;
    }

    uint16_t res_buf_free_space  = vpi_ringbuf_get_free_space(res_data_ctrl);
    uint16_t res_fifo_free_space = vpi_ringbuf_get_free_space(res_fifo_ctrl);
    // uart_printf("res_buf_free_space:%d",res_buf_free_space);
    // uart_printf("res_fifo_free_space:%d",res_fifo_free_space);

    if (res_buf_free_space < size + 2 ||
        res_fifo_free_space < sizeof(BleTransmitData)) {
        return VPI_ERR_FULL;
    }

    osal_lock_mutex(&res_mutex, 1000);

    vpi_ringbuf_push(res_data_ctrl, 2, (uint8_t *)&res_trans_id);
    vpi_ringbuf_push(res_data_ctrl, size, data);
    
    BleTransmitData item = { 0 };
    item.flags           = 0;
    item.length          = size + 2;
    item.opcode          = VS_CUSTOM_SPO2_MSG;
    item.data            = (void*)vpi_ringbuf_peek(res_data_ctrl, size + 2);

    vpi_ringbuf_push(res_fifo_ctrl, sizeof(BleTransmitData), (uint8_t *)&item);
    osal_unlock_mutex(&res_mutex);
    res_trans_id++;
    return VPI_SUCCESS;
}

void sending_task(void *param)
{
    int result = -1;
    void *sending_manager;
    result = sending_task_init();

    if (result != VPI_SUCCESS) {
        uart_printf("Fail to init sending task.\n"); // should be replaced
        osal_delete_task(NULL);
    }

    ble_sensor_service_callback_register(ble_handler);
    sending_manager = vpi_event_new_manager(COBJ_CUSTOM_MGR, sending_handler);

    vpi_event_register(EVENT_BLE_TRAN_REQ, sending_manager);

    uart_printf("Sending_task:listen.\n");

    while (1) {
        vpi_event_listen(sending_manager);
    }

    osal_delete_task(NULL);
}

int sending_handler(void *cobj, uint32_t event_id, void *data)
{
    switch (event_id) {
    case EVENT_BLE_TRAN_REQ: {
        int result = -1;

        if (ble_state == BLE_STATE_DISCONNECTED) {
            return VPI_ERR_NOT_READY;
        }

        SendParam *param = (SendParam *)data;
        if(current_ble_queue < 10){
            result = read_res_data(param->size, param->data);
        }else{
            result = 1;
        }
            
        osal_sem_post(&res_sem);
        if (result == VPI_SUCCESS) {
            // uart_printf("read res data finished.\n");
        } else {
            uart_printf("read res data failed.\n");
            return VPI_ERR_FULL;
        }

        BleTransmitData *ble_transmit_data =
            (BleTransmitData *)vpi_ringbuf_peek(res_fifo_ctrl,
                                                sizeof(BleTransmitData));

        result=ble_sensor_packet_send(ble_transmit_data);
        vpi_ringbuf_pop(res_data_ctrl, sizeof(Spo2Msg));
        vpi_ringbuf_pop(res_fifo_ctrl, sizeof(BleTransmitData));
        current_ble_queue++;
        if(result==BLE_EVENTS_OK)
        {
            // uart_printf("BLE_EVENTS_OK");
        }
        else
        {
            uart_printf("BLE_QUEUE_FULL");
        }
        break;
    }
    default:
        break;
    }

    return VPI_SUCCESS;
}

int ble_handler(uint8_t id, uint8_t *data)
{
    switch (id) {
    case EVENT_CONNECT_STATE: {
        if (*data == BLE_STATE_CONNECTED) {
            uart_printf("BLE:Connected.");
            ble_state = BLE_STATE_CONNECTED;

        } else if (*data == BLE_STATE_DISCONNECTED) {
            uart_printf("Disconnected.\n");
            ble_state = BLE_STATE_DISCONNECTED;
            current_ble_queue = 0;
        } else {
            // uart_printf("Other.\n");
        }

        break;
    }
    case EVENT_PDU_TRANSFER_STATUS: {
        // uart_printf("EVENT_PDU_TRANSFER_STATUS.\n");
        break;
    }
    case EVENT_BLE_DECODE_PACKET: {
        BleTransmitData *ble_transmit_data = (BleTransmitData *)data;

        if (ble_transmit_data->opcode == VS_CUSTOM_ACK_MSG) {
            //AckMessage *ack = (AckMessage *)ble_transmit_data->data;
            // uart_printf("ack");
            // uart_printf("%d  %d", ack->flag, ack->id);
            osal_lock_mutex(&res_mutex, 200);

            if(current_ble_queue>0) current_ble_queue--;

            osal_unlock_mutex(&res_mutex);
        }

        break;
    }
    default: {
        break;
    }
    }

    return VPI_SUCCESS;
}
