#include "sys_console.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Internal pointer to the actual hardware output function */
static Console_Output_Func pOutputRoute = NULL;

/**
 * @brief Initialize the console service by injecting the hardware output function.
 * @param output_func The function pointer to the actual hardware transmission API.
 */
void Console_Init(Console_Output_Func output_func) 
{
    pOutputRoute = output_func;
}

/**
 * @brief Output a raw string to the injected hardware interface.
 */
void Console_PutString(const char *str) 
{
    /* Safety check: Ensure the output route has been initialized and string is valid */
    if ((pOutputRoute != NULL) && (str != NULL) && (*str != '\0')) 
    {
        /* Route the data to the injected hardware function */
        pOutputRoute((uint8_t *)str, strlen(str));
    }
}

/**
 * @brief Format and output a string to the injected hardware interface.
 */
void Console_Printf(const char *format, ...) 
{
    char buffer[128]; 
    va_list args;

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Console_PutString(buffer);
}