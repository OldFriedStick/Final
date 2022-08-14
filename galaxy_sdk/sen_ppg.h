/**
 * @file sen_ppg.h
 * @author your name (you@domain.com)
 * @brief 处理ppg传感器相关事件
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _SEN_PPG_H_
#define _SEN_PPG_H_

#include"hal_ppg.h"


#define DEFAULT_SAMPLE_RATE 50
#define DEFAULT_LED_AMPLITUDE 100
#define DEFAULT_WORK_MODE (PPG_MODE_REDIR)

#define ERROR_OK 0
#define ERROR_INVALID 1

/**
 * @brief 初始化ppg传感器，设置默认工作参数
 * 当前版本中，PPG默认工作频率为50Hz，红光和红外共同模式
 */
void ppg_init();

/**
 * @brief 展示ppg当前配置信息
 * 
 */
void show_ppg_info();

void ppg_irq_enable();

void ppg_irq_disable();

/**
 * @brief ppg传感器开始工作，
 * 当接收到一定的数据后，比如内置缓冲区快满了
 * 会产生一个中断，中断处理函数为on_ppg_data_ready_handler
 * 此函数会产生一个EVENT_SEN_DATA_INTR事件
 * 请注册相应的manager处理数据，调用ppg_read_sample读取缓冲区中的数据
 * 注：ppg采样无法设置采样到足够的数据立马停止
 * 
 * @return int 
 */
int ppg_start_up();

/**
 * @brief 读取ppg传感器缓冲区数据
 * 
 * @param sample 数据接收缓冲区
 * @param exp_size 期望的读取数据个数，不要超过缓冲接收缓冲区
 * @param act_size 实际读到的数据个数
 * @return int 正常返回0
 */
int ppg_read_sample(uint16_t *samples, uint16_t exp_size, uint16_t *act_size);

/**
 * @brief ppg传感器停止工作
 * 
 * @return int 
 */
int ppg_shut_down();

int ppg_power(uint8_t enable);
#endif
