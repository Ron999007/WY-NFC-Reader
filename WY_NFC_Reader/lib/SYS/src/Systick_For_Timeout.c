/*****************************************************************************
* File Name          : Systick_For_Timeout.c
* Version            : V1.0.0.0 (Ported for M487)
* Description        : SysTick Implementation
******************************************************************************/   
#include "macro_utils.h"
#include "Systick_For_Timeout.h"

// variables
volatile uint16_t  i_Timeout_Count = 0;
volatile uint32_t  uwTick = 0;

// External function declaration
// Using __WEAK to prevent linker error if Client_Handler is not defined in main app
__WEAK void Client_Handler(void)
{
    // Do nothing if not defined externally
}

uint16_t get_Timeout_Count()
{
	return i_Timeout_Count;
}

/* * Logic to be executed every 1ms. 
 * This is called by the hardware exception SysTick_Handler.
 */
void Time_Handler()
{
	uwTick++;
	
	if(i_Timeout_Count > 0)
	{
		i_Timeout_Count -- ;	
	}

    /* Call the external handler (e.g., for LED/Buzzer tasks) */
	Client_Handler();
}

/**
  * @brief  SysTick Interrupt Handler (M487 Standard Name)
  */
void SysTick_Handler(void)
{
    Time_Handler();
}

uint8_t Systick_1ms_Init(void)
{
    /* Use CMSIS Standard SysTick Config
       SystemCoreClock is usually 192000000 (192MHz) for M487 
       Division by 1000 gives 1ms interrupt interval */
    if(SysTick_Config(SystemCoreClock / 1000))
    { 
        /* Capture error */ 
        return 0;
    }
    
    return 1;
}

void Systick_Timeout_Start(uint16_t Count_times)
{
	i_Timeout_Count = Count_times;
}

void Systick_Timeout_Stop(void)
{
    /* Usually strictly disabling SysTick is not recommended if 'Delay' relies on it.
       Here we just reset the software counter to 0. */
    i_Timeout_Count = 0;
}

uint8_t TimeOut_Check(void)
{		
    if(i_Timeout_Count > 0)
    {
        return  0; 
    }
    else
    {
        return  1;
    }
}

uint32_t Get_TickCount()
{
	return uwTick;
}

void Delay(uint32_t Delay)
{
    uint32_t tickstart = Get_TickCount();
    uint32_t wait = Delay;

    /* Add a freq to guarantee minimum wait */
    // The original logic subtracted 1 then added 1, simplifying logic here:
    if(wait < HAL_MAX_DELAY)
    {
        wait++;
    }

    while((Get_TickCount() - tickstart) < wait)
    {
      // Blocking wait
    }
}