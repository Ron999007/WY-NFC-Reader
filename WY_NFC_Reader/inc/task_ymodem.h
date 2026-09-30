/**
 * @file    task_ymodem.h
 * @brief   Always-on YMODEM Task Layer (SPI NOR Flash with OTA Config)
 */
#ifndef TASK_YMODEM_H
#define TASK_YMODEM_H

#include <stdint.h>
#include <stdbool.h>
#include "drv_snor_flash.h"

/* Memory Map Definitions */
#define OTA_CONFIG_ADDR       0x00000000  /* 4KB Sector reserved for OTA metadata */
#define FIRMWARE_UPDATE_ADDR  0x00001000  /* Actual firmware storage address */

/* Public Task APIs */
void Task_Ymodem_Init(void);
void Task_Ymodem(void);

#endif /* TASK_YMODEM_H */