#include "macro_utils.h"

// ---------------------------------------------
void Delay_ms(uint32_t MilliSec)
{
    // Uses the SysTick based blocking delay
    Delay(MilliSec);
}

// ---------------------------------------------
void Delay_us(uint32_t MicroSec)
{
    /* M487 Core Clock is usually 192 MHz.
       1 us = 192 cycles.
       Using a simple software loop for microsecond delay 
       to avoid conflict with the running SysTick interrupt.
    */
    
    volatile uint32_t cycles = MicroSec * (SystemCoreClock / 1000000);
    
    // Approximate loop overhead (divide by constant factor based on optimization)
    // A simple volatile decrement usually takes ~3-5 cycles depending on compiler.
    // We divide by 10 as a safe approximation for C-code loops without optimization interference.
    // For precise timing, consider using TIMER0 hardware delay if strict us timing is needed.
    
    cycles /= 8; // Tuning factor for 192MHz
    
    while(cycles--)
    {
        __NOP();
    }
}
 
// ---------------------------------------------
void Delay_s(uint32_t Second)
{
	Delay_ms(Second * 1000);
}
// ---------------------------------------------