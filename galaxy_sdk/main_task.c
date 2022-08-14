/**
 * @file main_task.c
 * @author your name (you@domain.com)
 * @brief 板子主任务循环
 * @version 0.1
 * @date 2022-08-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include"stdint.h"
#include "main_task.h"
#include"vpi_event.h"
#include"sen_imu.h"
#include"event_param.h"
#include"sen_tmp.h"
#include"sen_ecg.h"
#include"sen_ppg.h"
#include"osal_task_api.h"
#include"led.h"
#include "print.h"
#include "osal_lock_api.h"
#include "data_task.h"
#include "send.h"

/* 系统状态 */
static uint8_t sys_status;
OsalMutex status_lock;

int status_init(){
    osal_init_mutex(&status_lock);
    return 0;
}

/**
 * @brief Set the sys status object
 * 
 * @param status 
 * @return int 
 */
int set_status(uint8_t status){
    osal_lock_mutex(&status_lock, 100);
    sys_status = status;
    osal_unlock_mutex(&status_lock);
    return 0;
}

/**
 * @brief Get the sys status object
 * 
 * @param status 
 * @return int 
 */
int get_status(uint8_t *status){
    osal_lock_mutex(&status_lock, 100);
    *status = sys_status;
    osal_unlock_mutex(&status_lock);
    return 0;
}

static void *highg_manager_handler;

int highg_manager(void *cobj, uint32_t event_id, void *param){
    int ret = 0;
    if((uint32_t)param == INTR_HIGHG)
    {
        //进入MOVING状态
        set_status(MOVING);
    } 
    
    return ret;
}

/**
 * @brief 根据温度是否上升以及是否持续触碰人体判断是否穿戴
 * 
 * @return int 
 */
static int checking_if_take_on(){
    uint8_t lod_status = 0;
    uint8_t lod_conn = 0;
    uint16_t tmp0 = 0;
    uint16_t tmp1 = 0;
    int ret = 0;
    int i = 0;

    //开启相关传感器
    //TMP
    tmp_power_on();
    //tmp_init();

    //ECG
    ecg_power_on();
    //ecg_init();
    ecg_enable_lod_detect(1);

    //读取温度
    tmp_read(&tmp0);

    //检测是否持续接触
    for(i = 0; i <= 5; i++){
        ret = ecg_lod_detect(&lod_conn);
        if(lod_conn != ECG_LOD_OFF){
            lod_status++;
            if(lod_status > 2){
                ret = 1;
                break;
            }
        }   
        osal_sleep(500);
    }

    if(ret){
        //检测温度
        tmp_read(&tmp1);
        if(tmp0 > tmp1 - 5 && (tmp0 < 2800 || tmp0 > 4200)){
            ret = 0;
        }
    }
    return ret;
}

static void *take_off_manager;

int take_off_checker(void *cobj, uint32_t event_id, void *param){
    set_status(SLEEPING);
    return 0;
}


int power_off_sensors(){
    //PPG
    ppg_shut_down();
    ppg_power(0);
    //ECG
    ecg_power_off();
    //TMP
    tmp_power_off();

    return 0;
}

void main_task(void *param){
    uint8_t status = 0;
    uint16_t res[4] = {0};
    SendParam sendparam = {
        .data = (uint8_t *)res,
        .size = 8
    };
     /* 初始化ppg */
    ppg_init();
    /* 初始化ecg */
    ecg_init();
    /* 初始化温度传感器 */
    tmp_init();
    /* 初始imu */
    imu_init();
    /* 初始化状态锁 */
    status_init();
    highg_manager_handler = vpi_event_new_manager(COBJ_SEN_MGR, highg_manager);
    take_off_manager = vpi_event_new_manager(COBJ_SYS_MGR, take_off_checker);
    vpi_event_register(EVENT_SEN_IMU_TAP, highg_manager_handler);
    vpi_event_register(EVENT_SYS_STATE_UPDATE, take_off_manager);
    //设置加速度highg中断
    imu_init_highg();

    while(1){
        /* 状态机 */
        get_status(&status);
        switch (status)
        {
        case SLEEPING:
        /* 系统休眠，最低功耗运行 */
            //关闭所有传感器
            led_off();
            power_off_sensors();
        
            while(status == SLEEPING){
                imu_enable_highg_irq(1);
                vpi_event_listen(highg_manager_handler);
                get_status(&status);
            }
            break;
        case WORKING:
        /* 系统工作，即已穿戴，火力全开*/
            led_on();
            //开启传感器
            // ecg_power_on();
            // ecg_enable_lod_detect(1);
            ppg_power(1);
            ppg_init();

            //发送连接状态
            if(get_ble_state() == 1){
                res[3] = 1;
                send_res_data_req(&sendparam);
            }
            //通知开始SPO2任务
            start_collect_data();
            //持续检测是否摘下设备
            vpi_event_listen(take_off_manager);
            //摘下时，先通知停止SPO2任务然后切换系统状态
            uart_printf("take off");
            stop_collect_data();
            set_status(SLEEPING);
            
            //延时，防止误触连接
            osal_sleep(500);
            //发送连接状态
            if(get_ble_state() == 1){
                res[3] = 0;
                
                send_res_data_req(&sendparam);
            }
            
            break;
        case MOVING:
        /* 检测到加速度超过阈值，可能正在穿戴,检测是否确实在穿戴 */
            if(checking_if_take_on()){
                //已穿戴，进入工作状态
                set_status(WORKING);
            }else{
                //没有在穿戴，继续休眠
                set_status(SLEEPING);
            }
            break;
        case WAITING:
            /* code */
            break;
        default:
            break;
        }
    }
}

void *main_task_create(){
    return osal_create_task(main_task, "main", 1024, 4, NULL);
}