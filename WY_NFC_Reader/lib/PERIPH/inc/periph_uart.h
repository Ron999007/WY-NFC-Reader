/**
 * @file    periph_uart.h
 * @brief   M487 Specific UART Peripheral Driver Header.
 * @details Manages M487 hardware registers, ring buffers, and interrupt status.
 */
#ifndef __PERIPH_UART_H__
#define __PERIPH_UART_H__

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h"          /* Nuvoton BSP */
#include "utils_ringbuffer.h" /* Generic Ring Buffer implementation */

/**
 * @brief UART Object capturing hardware and software state for a specific port.
 */
typedef struct {
    UART_T *uart_base;        /* Pointer to the M487 hardware base (e.g., UART0, UART1) */
    Utils_RB_t *rx_rb;        /* Pointer to the RX ring buffer */
    Utils_RB_t *tx_rb;        /* Pointer to the TX ring buffer */
    volatile bool tx_running; /* Status flag: true if interrupt-driven TX is currently active */
} periph_uart_obj_t;

/**
 * @brief Initializes the UART hardware and binds software buffers to the object.
 * @param obj       Pointer to the UART object instance.
 * @param uart_base M487 Hardware base address (e.g., UART2).
 * @param baudrate  Desired baudrate (e.g., 115200).
 * @param rx_rb     Pointer to the initialized RX ring buffer.
 * @param tx_rb     Pointer to the initialized TX ring buffer.
 */
void periph_uart_init(periph_uart_obj_t *obj, UART_T *uart_base, uint32_t baudrate, Utils_RB_t *rx_rb, Utils_RB_t *tx_rb);

/**
 * @brief Checks the number of unread bytes in the RX ring buffer.
 * @param obj Pointer to the initialized UART object instance.
 * @return Number of bytes available to read.
 */
uint32_t periph_uart_available(periph_uart_obj_t *obj);

/**
 * @brief Pops one byte from the RX ring buffer.
 * @param obj  Pointer to the initialized UART object instance.
 * @param byte Pointer to the variable where the read byte will be stored.
 * @return true if successful, false if the buffer is empty.
 */
bool periph_uart_read(periph_uart_obj_t *obj, uint8_t *byte);

/**
 * @brief Pushes data into the TX ring buffer and triggers the TX interrupt if idle.
 * @param obj  Pointer to the initialized UART object instance.
 * @param data Pointer to the data array to transmit.
 * @param len  Number of bytes to transmit.
 */
void periph_uart_write(periph_uart_obj_t *obj, const uint8_t *data, uint32_t len);

/**
 * @brief Core UART Interrupt processing logic.
 * @details This function MUST be called inside the actual MCU hardware ISR (e.g., UART2_IRQHandler).
 * @param obj Pointer to the UART object associated with the interrupting port.
 */
void periph_uart_irq_process(periph_uart_obj_t *obj);

#endif /* __PERIPH_UART_H__ */