/**
 * @file main_task.h
 * @author your name (you@domain.com)
 * @brief 系统主任务
 * @version 0.1
 * @date 2022-08-09
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef _MAIN_TASK_H_
#define _MAIN_TASK_H_
enum Sys_Status {
    SLEEPING = 0,
    WORKING,
    MOVING,
    WAITING,
};

void *main_task_create();

int get_status(uint8_t *status);

int set_status(uint8_t status);

#endif