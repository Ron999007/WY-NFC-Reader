/***************************************************************************//**
 * @file     task_usb_vcom.h
 * @brief    USB Virtual COM Port Task Header
 ******************************************************************************/
#ifndef __TASK_USB_VCOM_H__
#define __TASK_USB_VCOM_H__

#include <stdint.h>
#include "macro_utils.h"

/* =======================================================================
 * CDC Class Specific Data Structures
 * ======================================================================= */
/* Line coding structure
  0-3 u32DTERate   Data terminal rate (baudrate), in bits per second
  4   u8CharFormat Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
  5   u8ParityType Parity:    0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
  6   u8DataBits   Data bits: 5, 6, 7, 8, 16  
*/
typedef struct
{
    uint32_t  u32DTERate;     /* Baud rate    */
    uint8_t   u8CharFormat;   /* Stop bit     */
    uint8_t   u8ParityType;   /* Parity       */
    uint8_t   u8DataBits;     /* Data bits    */
} STR_VCOM_LINE_CODING;

/* =======================================================================
 * USB Descriptor Configuration Macros
 * ======================================================================= */
#define USBD_VID                0x0416
#define USBD_PID                0xB002

#define SET_LINE_CODE           0x20
#define GET_LINE_CODE           0x21
#define SET_CONTROL_LINE_STATE  0x22

#define USBD_MAX_DMA_LEN        0x1000

/* EP maximum packet size */
#define CEP_MAX_PKT_SIZE        64
#define CEP_OTHER_MAX_PKT_SIZE  64
#define EPA_MAX_PKT_SIZE        512
#define EPA_OTHER_MAX_PKT_SIZE  64
#define EPB_MAX_PKT_SIZE        512
#define EPB_OTHER_MAX_PKT_SIZE  64
#define EPC_MAX_PKT_SIZE        64
#define EPC_OTHER_MAX_PKT_SIZE  64

/* EP Buffer Base Addresses in USB SRAM */
#define CEP_BUF_BASE            0
#define CEP_BUF_LEN             CEP_MAX_PKT_SIZE
#define EPA_BUF_BASE            0x200
#define EPA_BUF_LEN             EPA_MAX_PKT_SIZE
#define EPB_BUF_BASE            0x400
#define EPB_BUF_LEN             EPB_MAX_PKT_SIZE
#define EPC_BUF_BASE            0x600
#define EPC_BUF_LEN             EPC_MAX_PKT_SIZE

/* EP Numbers */
#define BULK_IN_EP_NUM          0x01
#define BULK_OUT_EP_NUM         0x02
#define INT_IN_EP_NUM           0x03

#define USBD_SELF_POWERED       0
#define USBD_REMOTE_WAKEUP      0
#define USBD_MAX_POWER          50  /* 50 * 2mA = 100mA */

/* =======================================================================
 * Exported APIs for Application / Console
 * ======================================================================= */
void Task_USB_VCOM_Init(void);

/* I/O Interfaces for Console Dependency Injection */
uint32_t VCOM_Read(uint8_t *buf, uint32_t max_len);
uint32_t VCOM_Write(uint8_t *buf, uint32_t len);

/* Internal Class Request Handler (used by hsusbd.c) */
void VCOM_ClassRequest(void);

#endif  /* __TASK_USB_VCOM_H__ */