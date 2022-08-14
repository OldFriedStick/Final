/**
 * @file sen_tmp.h
 * @author your name (you@domain.com)
 * @brief 温度传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _SEN_TMP_H_
#define _SEN_TMP_H_

#include <stdint.h>

/**
 * @brief 初始化温度传感器
 * 
 * @return int 
 */
int tmp_init();

/**
 * @brief 读取当前温度，一次读取一个
 * 
 * @param value 返回温度
 * @return int 
 */
int tmp_read(uint16_t *value);

/**
 * @brief 温度传感器电源
 * 
 * @param enable 1:on;0;off
 * @return int 
 */
int tmp_power(uint8_t enable);

#define tmp_power_on() tmp_power(1)
#define tmp_power_off() tmp_power(0)

#endif