/**
 * @file sen_ppg_ecg.c
 * @author your name (you@domain.com)
 * @brief 处理ppg传感器相关事务
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include<stdint.h>
#include<stddef.h>
#include "sen_ppg.h"
#include "hal_ppg.h"
#include "vpi_event.h"
#include "uart_printf.h"
#include "event_param.h"
//ppg设备句柄
static PpgDevice *ppg_device;

//当前工作模式,默认REDIR
static uint8_t cur_work_mode = DEFAULT_WORK_MODE;

char * work_mode_strings[] = {
    "IDLE",
    "RED",
    "IR",
    "REDIR",
};
//展示ppg配置信息
void show_ppg_info()
{   
    uint32_t rate = 0;
    uint8_t level_led = 0;
    
    hal_ppg_get_samplerate(ppg_device, &rate);
    hal_ppg_get_led_amplitude(ppg_device, &level_led, PPG_MODE_RED);
    
    uart_printf("PPG work mode  :%s", work_mode_strings[cur_work_mode & 0x3]);
    uart_printf("PPG sample rate:%d", rate);
    uart_printf("PPG led level  :%d", level_led);
}

void ppg_init()
{
    //获得ppg实例
    ppg_device = board_find_device_by_id(PPG_SENSOR_ID);

    //初始化ppg传感器
    hal_ppg_init(ppg_device);
    uart_printf("PPG inited!");

    //设置默认工作参数
    hal_ppg_set_samplerate(ppg_device, DEFAULT_SAMPLE_RATE);
    hal_ppg_set_work_mode(ppg_device, cur_work_mode);
    hal_ppg_set_led_amplitude(ppg_device, DEFAULT_LED_AMPLITUDE, cur_work_mode);
    hal_ppg_set_adc_range(ppg_device, 32768);
    hal_ppg_set_fifo_watermark(ppg_device, 0x10);

    //显示ppg信息
    show_ppg_info();
}

/**
 * @brief ppg采集数据中断处理函数
 * 
 */
static void on_ppg_data_ready_handler(){
    //读取中断状态
    //产生数据intr事件
    vpi_event_notify(EVENT_SEN_DATA_INTR, (void *)INTR_PPG);
}

int ppg_start_up(){
    //先开启中断，再唤醒设备
    hal_ppg_enable_interrupt(ppg_device, 1, on_ppg_data_ready_handler);
    hal_ppg_wake_up(ppg_device);
    return 0;
}

int ppg_read_sample(uint16_t *samples, uint16_t exp_size, uint16_t *act_size){
    int ret = hal_ppg_read_sample_bytes(ppg_device,
                                        (uint8_t *)samples,
                                        exp_size << 1,
                                        SENSOR_SAMPLE_2_BYTE_LITTLEENDIAN,
                                        act_size);
    //api所使用的数据大小是按字节算的，而我们按数据个数算，
    //所以要x2或除2
    *act_size = *act_size >> 1;
    return ret;
}   

void ppg_irq_enable(){
    hal_ppg_enable_interrupt(ppg_device, 1, on_ppg_data_ready_handler);
}

void ppg_irq_disable(){
    hal_ppg_enable_interrupt(ppg_device, 0, NULL);
}

int ppg_shut_down(){
    //先关闭中断，再关闭设备
    hal_ppg_enable_interrupt(ppg_device, 0, NULL);
    return hal_ppg_shut_down(ppg_device);
}

int ppg_power(uint8_t enable){
    return hal_ppg_enable_power(ppg_device, enable);
}