/**
 * @file data_struct_def.h
 * @author your name (you@domain.com)
 * @brief 自定义的数据结构
 * @version 0.1
 * @date 2022-07-26
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _DATA_STRUCT_DEF_H_
#define _DATA_STRUCT_DEF_H_
#include <stdint.h>
#include"osal_heap_api.h"
//使用 MALLOC 替换malloc，方便替换成osal_malloc
#define MALLOC osal_malloc

//同上
#define FREE osal_free

//模减
#define Mod_Sub(a, b, mod)  ((a) + (mod) - (b)) % (mod)

//模加
#define Mod_Add(a, b, mod)  ((a) + (b)) % (mod)


typedef struct _ring_array
{
    uint16_t size;  //循环数组大小
    uint16_t tail;  //数组指针
    int64_t total; //数组中数据总和
    uint8_t  isfull;//数组是否已满
    int      *data; //数组数据
} RingArray;

typedef enum PpgDataFormat {

    GREEN_FORMAT = 0, /**< Format: Green, Green, Green */

    RED_IR_FORMAT = 1, /**< Format: Red,IR, Red,IR, Red,IR */

    RED_FORMAT = 2, /**< Format: Red, Red, Red */

    IR_FORMAT = 3, /**< Format: IR, IR, IR */

} PpgDataFormat;

typedef struct _PpgData {

    PpgDataFormat data_format; /**< Data format, @see PpgDataFormat */

    uint16_t sample_rate; /**< 50Hz by default */

    uint16_t size; /**< data size in byte */

    uint16_t *data; /**< data buffer */

} PpgData;


/**
 * @brief 初始化循环数组,
 * 
 * @param a 要初始化的循环数组结构体
 * @param buffer 数据内存
 * @param size 数组大小
 * @return int 0 if success
 */
int alloc_RingArray(RingArray *a, uint16_t size);

/**
 * @brief 重置循环数组
 * 
 * @param a 
 */
void reset_RingArray(RingArray *a);

/**
 * @brief 释放循环数组的内存
 * 
 * @param a 
 */
void free_RingArray(RingArray *a);
/**
 * @brief 在数组a中插入值
 * 
 * @param a 
 * @param value 
 */
void ra_insert(RingArray *a, int value);

/**
 * @brief 删除一个数据
 * 
 * @param a 
 */
void ra_del(RingArray *a);

/**
 * @brief 对数组求符号，
 * 
 * @param src 需要求符号的数组
 * @param dst 保存求符号结果的数组,大小必须比源数组大
 * @return int 返回0表示正常
 */
int sign(RingArray *src, RingArray *dst);

/**
 * @brief 对源数组求差分，从源数组当前指针开始差分
 * 
 * @param src 源数组，当源数组指针从0开始时，目标数组可以和源数组是同一个
 * @param dst 目的数组，差分结果从目的数组的0索引开始
 * @return int 0表示正常
 */
int cycle_diff(RingArray *src, RingArray *dst);

#endif