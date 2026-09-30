#ifndef __MY_APPLICATION_H__
#define __MY_APPLICATION_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "macro_utils.h"
#include "mid_shell.h"
#include "mid_ymodem.h"
#include "mid_mqtt.h"
#include "ff.h"
#include "sys_console.h"
#include "drv_bc45b4522.h"
#include "drv_button.h"
#include "drv_rgb.h"
#include "drv_led.h"
#include "drv_snor_flash.h"
#include "drv_esp8266.h"
#include "drv_oled_ssd1306.h"
#include "drv_sd_card.h"
#include "drv_buzzer.h"
#include "drv_dht11.h"
#include "drv_eeprom.h"
#include "drv_rs232.h"
#include "drv_rs485.h"
#include "diskio.h"
#include "periph_spi.h"
#include "periph_uart.h"
#include "periph_pwm.h"
#include "periph_i2c.h"
#include "periph_can.h"
#include "utils_ringbuffer.h"
#include "task_led.h"
#include "task_rgb_led.h"
#include "task_button.h"
#include "task_console.h"
#include "task_usb_vcom.h"
#include "task_snor_flash.h"
#include "task_ymodem.h"
#include "task_wifi.h" 
#include "task_nfc_reader.h"
#include "task_oled.h"
#include "task_sdcard.h"
#include "task_buzzer.h"
#include "task_dht11.h"
#include "task_eeprom.h"
#include "task_rs232.h"
#include "task_rs485.h"
#include "task_can.h"
#include "image.h"


#define BTN_SW2         PG15
#define BTN_SW3         PF11
#define LED_R           PB7
#define LED_G           PB8
#define NFC_RESET       PB6
#define NFC_IRQ         PB1
#define CAN1_TERM_EN    PF6



void MyApplication_Init(void);
void MyApplication_Run(void);

#endif

