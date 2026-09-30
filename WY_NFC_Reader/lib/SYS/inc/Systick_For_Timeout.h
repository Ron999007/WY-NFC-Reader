/*****************************************************************************
* File Name          : Systick_For_Timeout.h
* Version            : V1.0.0.0 (Ported for M487)
* Description        : SysTick Timeout Header
******************************************************************************/   

#ifndef __SYSTICK_FOR_TIMEOUT_H 
#define __SYSTICK_FOR_TIMEOUT_H

#include "NuMicro.h"

#define HAL_MAX_DELAY      	0xFFFFFFFFU
#define HAL_DELAY      		1000
#define Timeout_Count       get_Timeout_Count()

uint8_t 	Systick_1ms_Init(void);
uint8_t 	TimeOut_Check(void);
uint16_t 	get_Timeout_Count(void);
uint32_t 	Get_TickCount(void);
void 		Delay(uint32_t Delay);
void    	Systick_Timeout_Start(uint16_t Count_times);
void    	Systick_Timeout_Stop(void);

/* * This function is called every 1ms in SysTick_Handler.
 * Defined in Systick_For_Timeout.c
 */
void 		Time_Handler(void);

#endif