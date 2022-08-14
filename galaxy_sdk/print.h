/**
 * @file print.h
 * @author your name (you@domain.com)
 * @brief 统一打印接口
 *  当使用zprobe调试时，先定义宏ZPROBE再include此头文件
 * @version 0.1
 * @date 2022-08-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include"uart_printf.h"
#include"stdio.h"
// #define ZPROBE

#ifdef ZPROBE

    #define print(...) printf(__VA_ARGS__)

#else

    #define print(...) uart_printf(__VA_ARGS__)
#endif

