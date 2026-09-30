#ifndef __SYS_CONSOLE_H__
#define __SYS_CONSOLE_H__

#include <stdint.h>

/* * Define a function pointer type for the hardware output routing.
 * It matches the signature of Periph_UART_Write (uint8_t*, uint32_t).
 */
typedef void (*Console_Output_Func)(uint8_t *data, uint32_t len);

/* API Declarations */
void Console_Init(Console_Output_Func output_func);
void Console_PutString(const char *str);
void Console_Printf(const char *format, ...);

#endif /* __SYS_CONSOLE_H__ */