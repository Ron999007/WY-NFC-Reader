#ifndef __MACRO_UTILS_H__
#define __MACRO_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "NuMicro.h"
#include "main.h"
#include "Systick_For_Timeout.h"
#include "Delay.h" 

#ifdef  USE_DEBUG_PRINT  
    #define DBG_PRINT(...)  printf(__VA_ARGS__)
#else
    #define DBG_PRINT(...)  do {} while(0)
#endif
        


#define INT_USE_CHECK_REG 			1		//polling reg07 irq bit or irq pin?

#define NFC_DEBUG					0		//printf debug information

#define	INTF_UART					0
//#define INTERFACE_UART              1		//use uart interface
	#define	BR	BR1228800
	#define NBR	1228800

	#define BR14400		0xDA
	#define	BR19200		0xCB
	#define	BR38400		0xAB
	#define	BR57600		0x9A
	#define	BR115200	0x7A
	#define	BR128000	0x74
	#define	BR230400	0x5A
	#define	BR460800	0x3A
	#define	BR921600	0x1C
	#define	BR1228800	0x15

#define INTF_SPI					1		//use spi interface
//#define INTERFACE_SPI				0		//use spi interface

#define INTF_I2C					2		//use spi interface
//#define INTERFACE_I2C               0		//use i2c interface
	#define I2C_SLAVE_ADDR			(0x50>>1)

/*********************************************/
#define FOSC 				22118400ul

#define BAUD 				115200

//typedef unsigned long 		uint32_t;
//typedef unsigned short		uint16_t;
//typedef unsigned char 		uint8_t;
//typedef unsigned char 		bool;
typedef unsigned long 		tick;

typedef unsigned long 		U32;
typedef unsigned short 		U16;
typedef unsigned char 		U8;

typedef float float32_t;

//#define FALSE 				0
//#define TRUE				1

//#define BIT0				0x01
//#define BIT1				0x02
//#define BIT2 				0x04
//#define BIT3 				0x08
//#define BIT4 				0x10
//#define BIT5 				0x20
//#define BIT6 				0x40
//#define BIT7 				0x80


//command between pc sw & device
#define COM_PKT_CMD_READ_REG 					0x01
#define COM_PKT_CMD_WRITE_REG					0x02
#define COM_PKT_CMD_QUERY_MODE					0x0D	//manual or auto card serach
#define COM_PKT_CMD_CHIP_RESET      			0x0E    //chip software reset
#define COM_PKT_CMD_CARD_TYPE					0x0F
#define COM_PKT_CMD_HALT						0x11 	//send halt command
#define COM_PKT_CMD_STATISTICS					0x12 	//statistics information 
#define COM_PKT_CMD_LPCD_CONFIG_TEST 			0x16 	//test lpcd function
#define COM_PKT_CMD_LPCD_CONFIG_TEST_STOP 		0x17
#define COM_PKT_CMD_POWERON						0x18
#define COM_PKT_CMD_POWERDOWN					0x19
#define	COM_PKT_CMD_MIFARE_RWPOLLING			0x54








#endif