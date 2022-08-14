/**
 * @file sen_tmp.c
 * @author your name (you@domain.com)
 * @brief 文档传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "sen_tmp.h"
#include "hal_tmp.h"
#include "board.h"

static TmpDevice *tmp_device;

int tmp_init(){
    tmp_device = board_find_device_by_id(TMP_SENSOR_ID);
    return hal_tmp_init(tmp_device);
}

int tmp_read(uint16_t *value){
    hal_tmp_set_mode(tmp_device, TMP_ONESHOT);
    return hal_tmp_read(tmp_device, value);
}

int tmp_power(uint8_t enable){
    return hal_tmp_power(tmp_device, enable);
}
