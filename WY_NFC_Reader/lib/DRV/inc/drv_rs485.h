#ifndef DRV_RS485_H
#define DRV_RS485_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the RS485 hardware, UART peripheral, and ring buffers.
 */
void drv_rs485_init(void);

/**
 * @brief Transmit data over the RS485 bus safely in half-duplex mode.
 * * @param data Pointer to the data buffer to be transmitted.
 * @param length Number of bytes to transmit.
 */
void drv_rs485_transmit(const uint8_t *data, uint32_t length);

/**
 * @brief Get the number of available bytes in the RX ring buffer.
 * * @return Number of unread bytes.
 */
uint32_t drv_rs485_available(void);

/**
 * @brief Read a single byte from the RX ring buffer.
 * * @param byte Pointer to store the received byte.
 * @return true if a byte was successfully read, false if the buffer is empty.
 */
bool drv_rs485_read(uint8_t *byte);

#endif /* DRV_RS485_H */