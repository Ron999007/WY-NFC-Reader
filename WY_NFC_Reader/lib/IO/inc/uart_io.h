/**
 * @file    uart_io.h
 * @brief   Abstract UART Communication Interface (The Contract).
 * @details This header defines a generic interface for UART communication. 
 * It decouples device drivers from specific MCU hardware.
 */
#ifndef __UART_IO_H__
#define __UART_IO_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Generic UART IO Interface structure.
 * @details Drivers will use this structure to interact with the underlying hardware.
 */
typedef struct {
    /** * @brief Opaque pointer to the hardware context.
     * @details Usually points to a peripheral object (e.g., periph_uart_obj_t).
     */
    void *user_data; 

    /** * @brief Checks how many bytes are available in the RX buffer.
     * @param user_data Pointer to the hardware context.
     * @return Number of bytes ready to be read.
     */
    uint32_t (*available)(void *user_data);

    /** * @brief Reads a single byte from the RX buffer.
     * @param user_data Pointer to the hardware context.
     * @param byte Pointer to store the received byte.
     * @return true if a byte was successfully read, false if the buffer was empty.
     */
    bool (*read)(void *user_data, uint8_t *byte);

    /** * @brief Transmits a data array via UART.
     * @param user_data Pointer to the hardware context.
     * @param data Pointer to the buffer to transmit.
     * @param len  Length of the data buffer.
     */
    void (*write)(void *user_data, const uint8_t *data, uint32_t len);

    /** * @brief Gets the current system tick count in milliseconds.
     * @return Current system tick value.
     */
    uint32_t (*get_tick)(void);

    /** * @brief Controls the hardware Reset pin of the connected device.
     * @param user_data Pointer to the hardware context.
     * @param level true to pull the pin HIGH, false to pull it LOW.
     */
    void (*set_reset_pin)(void *user_data, bool level);
} uart_io_interface_t;

#endif /* __UART_IO_H__ */