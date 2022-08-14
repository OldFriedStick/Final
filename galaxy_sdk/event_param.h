
#ifndef _EVENT_PARAM_H_
#define _EVENT_PARAM_H_
#include"stdint.h"
//中断类型
#define INTR_NULL 0U
#define INTR_PPG  1U
#define INTR_HIGHG 2U
#define INTR_ACCEL 3U

typedef struct _sample_data
{
    uint8_t label;          //当前标签
    uint16_t data[256];     //数据
    uint8_t labels[256];    //数据标签
    uint16_t cnt;           //数据个数
} SampleData;

typedef struct _lose_data
{
    uint64_t time_b;        //开始时间
    uint64_t time_e;        //结束时间
    uint16_t sample_rate;   //采样频率
    uint64_t cnt;           //总采样个数
    uint16_t lose_rate;     //丢失率,千分数
}LoseData;

typedef struct _wrong_data
{
    uint64_t sample_cnt;    //累计采样个数
    uint64_t wrong_cnt;     //错误处理个数
    uint16_t wrong_rate;    //错误率，千分数
}WrongData;

#endif