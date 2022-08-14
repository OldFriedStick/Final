/**
 * @file sen_imu.h
 * @author your name (you@domain.com)
 * @brief IMU传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _SEN_IMU_H_
#define _SEN_IMU_H_
#include"hal_imu.h"

int imu_init();

int imu_init_highg();

int imu_set_interrupt(uint8_t enable, uint8_t irq_type, IMUInterruptSetting *irq_settings);

int imu_enable_highg_irq(uint8_t enable);

int imu_flush();
#endif