/**
 * @file sen_ecg.c
 * @author tkyzp
 * @brief ecg传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "sen_ecg.h"
#include "board.h"

static EcgDevice *ecg_device;

int ecg_init(){
    ecg_device = board_find_device_by_id(ECG_SENSOR_ID);
    return hal_ecg_init(ecg_device);
}

int ecg_enable_lod_detect(uint8_t enable){
    return hal_ecg_enable_lod_detect(ecg_device, enable);
}

int ecg_lod_detect(uint8_t *connected){
    return hal_ecg_lod_detect(ecg_device, connected);
}

int ecg_power(uint8_t enable){
    return hal_ecg_power(ecg_device, enable);
}