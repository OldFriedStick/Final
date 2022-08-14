/**
 * Copyright (C) 2022 VeriSilicon Holdings Co., Ltd.
 * All rights reserved.
 *
 * @file main.c
 * @brief Functions for main
 * @author Shaobo Tu <Shaobo.Tu@verisilicon.com>
 */

#include <stdint.h>
#include "vs_conf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "soc_init.h"
#include "hal_bsp.h"
#include "uart_printf.h"
#include "vs_ble.h"
#include "osal_task_api.h"
#include "vpi_storage.h"
#include "vpi_error_code.h"
#include "vpi_error.h"
#include "sample_board.h"

#include "led.h"
#include "sen_ppg.h"
#include "sen_ecg.h"
#include "sen_tmp.h"
#include "sen_imu.h"
#include "main_task.h"
#include "event_param.h"
#include "vpi_event.h"
#include "data_task.h"
#include "osal_time_api.h"
#include "send.h"

static const BoardOperations *board_ops = &sample_board_ops;
static BoardDevice board_dev;


int main(void)
{
    int status;

    /* Initialize soc */
    status = soc_init();
    status = vsd_to_vpi(status);

    /* Initialize uart */
    uart_debug_init();

    if (status == VPI_ERR_LICENSE) {
        uart_printf("Invalid SDK license!\r\n");
        return status;
    }

    /* Initialize bsp */
    bsp_init();
    /* Initialize board */
    board_register(board_ops);
    board_init((void *)&board_dev);
    if (board_dev.name)
        uart_printf("Board: %s", board_dev.name);
    /* show current image information */
    vpi_storage_show_img_info();
    /* Start ble */
    vs_ble_start();
    uart_printf("Hello VeriHealth!\r\n");

    /* 初始化LED灯 */
    led_init();
    //创建任务
    main_task_create();
    data_task_create();
    osal_create_task(sending_task, "send", 512, 5, NULL);
    /* Start os */
    vTaskStartScheduler();

    return 0;
}
