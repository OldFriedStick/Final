/**
 * @file sen_imu.c
 * @author your name (you@domain.com)
 * @brief IMU传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include"stddef.h"
#include"sen_imu.h"
#include"board.h"
#include"vpi_event.h"
#include"event_param.h"

static ImuDevice *imu_device;

int imu_init(){
    // imu_device = board_find_device_by_id(IMU_SENSOR_ID);
    // hal_imu_init(imu_device);
    imu_device = board_find_device_by_id(IMU_SENSOR_ID);
    return hal_imu_init(imu_device);   
}



int imu_set_interrupt(uint8_t enable, uint8_t irq_type, IMUInterruptSetting *irq_settings){
    return hal_imu_set_interrupt_cfg(imu_device, enable, irq_type, irq_settings);
}

int imu_enable_interrupt(uint8_t pin, uint8_t enable, ImuDataReadyHandler data_ready_callback){
    return hal_imu_enable_interrupt(imu_device, pin, enable, data_ready_callback);
}

int imu_init_highg(){
    hal_imu_set_low_power(imu_device, IMU_ACCEL);
    hal_imu_set_stop(imu_device, IMU_GYRO);
    //配置阈值
    IMUHighGCondition condition = {
        .threshold = 1,
        .duration = 30,
        .hysteresis = 0
    };
    hal_imu_set_highg_threshold(imu_device, condition);
    //配置highg中断
    return imu_set_interrupt(1, IMU_ACC_HIGH_G_INTERRUPT, NULL); 
}

#include "print.h"

static void default_highg_irq(){
    //关中断
    imu_enable_interrupt(IMU_WAKE_PIN, 0, NULL);
    print("highg \r\n");
    //通知事件
    vpi_event_notify(EVENT_SEN_IMU_TAP, (void *)INTR_HIGHG);
}


int imu_enable_highg_irq(uint8_t enable){
    void *irq_handler = enable? default_highg_irq : NULL;
    return imu_enable_interrupt(IMU_WAKE_PIN, enable, irq_handler);
}

int imu_flush(){
    return hal_imu_flush_fifo(imu_device);
}