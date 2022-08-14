/**
 * @file data_task.c
 * @author your name (you@domain.com)
 * @brief 数据处理相关任务
 * @version 0.1
 * @date 2022-07-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "data_task.h"
#include "osal_task_api.h"
#include "vpi_event.h"
#include "uart_printf.h"
#include "vpi_ringbuffer.h"
#include "osal_heap_api.h"
#include "sen_ppg.h"
#include "osal_semaphore.h"
#include "data_struct_def.h"
#include "osal_lock_api.h"
#include "event_param.h"
#include "spo2.h"
#include "osal_time_api.h"
#include "vpi_sw_timer.h"
#include "send.h"
#include "main_task.h"

static OsalSemaphore sem_ppg_data;
static OsalMutex lock_ppg_irq;

void *task_collect_handler;     //数据收集任务句柄
void * task_process_handler;    //数据处理任务句柄
void * timer_rate_updater;      //定时器
void * timer_res_sender;        //蓝牙发送定时器

SampleData sample_data;         //采样数据缓冲区

LoseData lose_data;             //数据丢失率相关

uint8_t is_first_data = 1;

WrongData wrong_data;           //错误率相关

uint16_t spo2;                  //保存当前的spo2
uint16_t heart_rate;            //缓存当前的心率
uint8_t keep_collect;           //是否继续收集数据

/**
 * @brief 开始采集数据
 * 
 * @return int 
 */
int start_collect_data(){
    uart_printf("start collect!");
    if(keep_collect){
        //正在采集
        return 0;
    }
    //重置缓存的数据
    spo2_reset();
    spo2 = 0;

    //初始化错误率更新定时器
    vpi_timer_reset(timer_rate_updater, 100);
    vpi_timer_reset(timer_res_sender, 100);

    lose_data.cnt = 0;
    lose_data.sample_rate = 100;//红光加红外
    lose_data.lose_rate = 0;
    

    wrong_data.sample_cnt = 0;
    wrong_data.wrong_cnt = 0;
    wrong_data.wrong_rate = 0;

    osal_lock_mutex(&lock_ppg_irq, 100);
    keep_collect = 1;
    vpi_timer_start(timer_rate_updater, 100);
    vpi_timer_start(timer_res_sender, 100);
    lose_data.time_b = osal_get_uptime();
    lose_data.time_e = lose_data.time_b;
    ppg_start_up();
    osal_unlock_mutex(&lock_ppg_irq);
    
    return 0;
}

/**
 * @brief 停止采集数据
 * 
 * @return int 
 */
int stop_collect_data(){
    uart_printf("stop collect!");
    osal_lock_mutex(&lock_ppg_irq, 100);
    keep_collect = 0;
    ppg_shut_down();
    vpi_timer_stop(timer_res_sender, 100);
    vpi_timer_stop(timer_rate_updater, 100);
    osal_unlock_mutex(&lock_ppg_irq);
    return 0;
}

/* 错误率更新定时处理函数 */
void rate_updater(void *self){
    uint64_t exp;
    exp = lose_data.time_e - lose_data.time_b;
    exp = exp * lose_data.sample_rate / 1000;
    if(exp > 1200) exp = 1000;
    //更新采样丢失率
    if(lose_data.cnt > exp || lose_data.cnt == 0){
        lose_data.lose_rate = 0;
    }else{
        lose_data.lose_rate = (exp - lose_data.cnt) * 1000 / exp;
    }
    
    //更新数据错误率
    if(wrong_data.sample_cnt != 0) wrong_data.wrong_rate = wrong_data.wrong_cnt * 1000 / wrong_data.sample_cnt;
    lose_data.cnt = 0;
    is_first_data = 1;
    lose_data.time_b = lose_data.time_e;
    
}

static uint8_t leave_state;

//PPG数据收集，当PPG数据中断触发时会触发此事件
static int data_collect_manager(void *cobj, uint32_t event_id, void *param){
    u_int16_t act_size = 0;
    //检查是否ppg数据中断
    if(!((uint32_t)param == INTR_PPG)){
        return 0;
    }

    //先读取PPG的缓存数据到我们的缓存中
    //已知PPG中FIFO的缓存数据个数为0x20
    ppg_read_sample(sample_data.data + sample_data.cnt, 0x20, &act_size);
    //记录采样数据个数和时间
    if(act_size == 0){
        goto wait_next;
    }
    lose_data.time_e = osal_get_uptime();
    if(is_first_data && act_size != 0){
        lose_data.time_b = lose_data.time_e;
        is_first_data = 0;
    }
    lose_data.cnt += act_size;
    
    //记录总采样个数
    wrong_data.sample_cnt += act_size;
    //设置数据lable
    memset(sample_data.labels + sample_data.cnt, sample_data.label, act_size);
    sample_data.cnt += act_size;
    
    //当未处理数据数据达到达到一秒时，对数据进行处理
    if(sample_data.cnt >= 100){
        //抽样数据，检查有效性，主要是检查人体是否离开
        if(sample_data.data[sample_data.cnt / 2] < 200){
            leave_state++;
        }else{
            leave_state = 0;
        }

        if(leave_state > 1) 
        {
            vpi_event_notify(EVENT_SYS_STATE_UPDATE, NULL);
        }else{
            //通知数据一秒钟的数据已准备就绪
            vpi_event_notify(EVENT_SEN_DATA_READY, &sample_data);
            //等待数据同步
            osal_sem_wait(&sem_ppg_data, 100);
        }

        //重置cnt，更改lable，等待下一次数据量达到一秒
        sample_data.cnt = 0;
        sample_data.label++;
    }else{
        //开启PPG中断等待下次处理
        //每隔0.1秒开启一次，避免太频繁触发中断
        osal_sleep(100);
    }

wait_next:
    //原子操作，避免多个任务同时开关中断；
    osal_lock_mutex(&lock_ppg_irq, 100);
    if(keep_collect) ppg_irq_enable();
    osal_unlock_mutex(&lock_ppg_irq);
    return 0;
}


static void data_collect_task(void *param)
{
    //注册EVENT_SEN_DATA_intr事件
    void *manager = vpi_event_new_manager(COBJ_SEN_MGR, data_collect_manager);
    vpi_event_register(EVENT_SEN_DATA_INTR, manager);

    //数据处理循环
    while(1){
        vpi_event_listen(manager);
    }

    data_task_delete();
}

/**
 * @brief 数据处理，当收集到一定数据的时候触发此manager
 * 
 * @param cobj 
 * @param event_id 
 * @param param 
 * @return int 
 */
static int data_ready_manager(void *cobj, uint32_t event_id, void *param){
    int s = 0;
    int ret = 0;

    //预处理数据
    //算法处理嵌入式版算法略有不同，
    //为了及时释放信号量将数据预处理单独摘出来
    //需要计算数据错误率
    wrong_data.wrong_cnt += preprocess_data(param);

    //已经不需要再使用临时缓冲区，
    //释放信号量
    osal_sem_post(&sem_ppg_data);

    // 计算spo2，ppg数据已经预处理，所以此处可以传NULL
    //  跟C模拟代码略有不同
    ret = spo2_process(NULL, &s);

    //如果数据有效，更新当前spo2值
    if(!ret) spo2 = s;
    
    return ret;
}

void res_sender(void * self){
    SendParam send_param = {0};
    uint16_t data[4];
    //发送结果数据包
    if(get_ble_state() == 1){
        data[0] = spo2;
        data[1] = lose_data.lose_rate;
        data[2] = wrong_data.wrong_rate;
        get_status((uint8_t *)(data + 3)); 
        send_param.data = (uint8_t *)data;
        send_param.size = 8;
        
        send_res_data_req(&send_param);
    }else{
        uart_printf("lose_rate[%u];wrong_rate[%u];spo2[%u];cnt[%llu]",
                             lose_data.lose_rate,
                             wrong_data.wrong_rate,
                             spo2, lose_data.cnt);
    }
}

/**
 * @brief 
 * 
 * @param param 
 */
static void data_ready_task(void *param){
    //初始化算法
    spo2_init();
    uint32_t id;

    void *manager = vpi_event_new_manager(COBJ_ALGO_MGR, data_ready_manager);

    vpi_event_register(EVENT_SEN_DATA_READY, manager);

    //初始化软件定时器
    vpi_timer_init();
    //创建软件定时器
    timer_rate_updater = vpi_timer_create("rate_updater", VS_TIMER_SW_REPEAT, &id, 10000, rate_updater);
    timer_res_sender = vpi_timer_create("res_sender", VS_TIMER_SW_REPEAT, NULL, 1000, res_sender);
    while(1){
        vpi_event_listen(manager);
    }
}

void data_task_create(){
    //初始化全局变量
    osal_init_sem(&sem_ppg_data);
    osal_init_mutex(&lock_ppg_irq);

    task_collect_handler = osal_create_task(data_collect_task, "data_collect", 1024, 5, NULL);
    task_process_handler = osal_create_task(data_ready_task, "data_process", 1024, 3, NULL);
}

void data_task_delete(){
    osal_delete_task(task_collect_handler);
    osal_delete_task(task_process_handler);
    task_collect_handler = NULL;
    task_process_handler = NULL;
}
