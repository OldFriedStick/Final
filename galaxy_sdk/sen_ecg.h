/**
 * @file sen_ecg.h
 * @author tkyzp
 * @brief ecg传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _SEN_ECG_H_
#define _SEN_ECG_H_
#include <stdint.h>
#include "hal_ecg.h"

int ecg_init();

/**
 * @brief 使能lod检测功能
 * 
 * @param enable 1:enable;0:disable
 * @return int 
 */
int ecg_enable_lod_detect(uint8_t enable);

#define ecg_lod_enable() ecg_enable_lod_detect(1)
#define ecg_lod_disable() ecg_enable_lod_detect(0)

/**
 * @brief 读取lod状态
 * 
 * @param connected 返回的lod状态
 * @return int 
 */
int ecg_lod_detect(uint8_t *connected);

/**
 * @brief 设置ecg电源
 * 
 * @param enable 1:on;0:off
 * @return int 
 */
int ecg_power(uint8_t enable);

#define ecg_power_on() ecg_power(1)
#define ecg_power_off() ecg_power(0)

#endif