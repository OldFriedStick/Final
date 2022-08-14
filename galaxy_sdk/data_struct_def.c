#include "data_struct_def.h"
/**
 * @brief 初始化循环数组,
 * 
 * @param a 要初始化的循环数组结构体
 * @param buffer 数据内存
 * @param size 数组大小
 * @return int 0 if success
 */
int alloc_RingArray(RingArray *a, uint16_t size){
    int ret = 0;
    int *buffer = NULL;

    if(a){
        buffer = (int *)MALLOC(size * sizeof(int));

        if(buffer == NULL)
        {
            ret = 2;
        }
        else
        {
            a->data = buffer;
            a->size = size;
            a->isfull = 0;
            a->tail = 0;
            a->total = 0;
        }
    }else{
        ret = 1;
    }
    return ret;
}

/**
 * @brief 重置循环数组
 * 
 * @param a 
 */
void reset_RingArray(RingArray *a){
    if(a){
        a->isfull = 0;
        a->tail = 0;
        a->total = 0;
    }
}

/**
 * @brief 释放循环数组的内存
 * 
 * @param a 
 */
void free_RingArray(RingArray *a){
    if(a){
        FREE(a->data);
        a->data = NULL;
    }
}

/**
 * @brief 在数组a中插入值
 * 
 * @param a 
 * @param value 
 */
void ra_insert(RingArray *a, int value){
    //暂存即将被覆盖的值
    int tmp = a->data[a->tail % a->size];
    a->data[a->tail++ % a->size] = value;
    if(a->tail >= a->size){
        a->isfull = 1;
        a->tail %= a->size;
    }
    if(a->isfull){
        value -= tmp;
    }
    a->total += value;
}

/**
 * @brief 删除一个数据
 * 
 * @param a 
 */
void ra_del(RingArray *a){
    a->tail = Mod_Sub(a->tail, 1, a->size);
    a->total -= a->data[a->tail];
    //删去的值设为0，不影响下次添加数的时候计算总值
    a->data[a->tail] = 0;
}

/**
 * @brief 对数组求符号，
 * 
 * @param src 需要求符号的数组
 * @param dst 保存求符号结果的数组,大小必须比源数组大
 * @return int 返回0表示正常
 */
int sign(RingArray *src, RingArray *dst){
    int ret = 0;
    int i = 0;
    int size = src->size;

    if(src && dst && src->size <= dst->size)
    {
        //从头到尾，按次序求符号
        for(i = 0; i < size; i++){
            //当小于0时 = -1，
            //由于c语言判断正确为1，错误为0，所以大于0时src[i] > 0判断正确，值为1，否则为0
            dst->data[i] = src->data[i] < 0 ? -1 : (src->data[i] > 0);
        }

    }else{
        ret = 1;
    }

    return ret;
}

/**
 * @brief 对源数组求差分，从源数组当前指针开始差分
 * 
 * @param src 源数组，当源数组指针从0开始时，目标数组可以和源数组是同一个
 * @param dst 目的数组，差分结果从目的数组的0索引开始
 * @return int 0表示正常
 */
int cycle_diff(RingArray *src, RingArray *dst){
    int base = src->tail;
    int i = 0;
    int ret = 0;

    if(src && dst && src->size <= dst->size)
    {
        for(i = 0; i < src->size - 1; i++){
           dst->data[i] = src->data[(base + i + 1) % src->size]
                            - src->data[(base + i) % src->size];
        }
        
        dst->tail = 0;
    }else{
        ret = 1;
    }
    return ret;
}