#ifndef DRV_RS232_H
#define DRV_RS232_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief initialize the rs232 hardware, uart peripheral, and ring buffers.
 */
void drv_rs232_init(void);

/**
 * @brief transmit data over the rs232 bus.
 *
 * @param data pointer to the data buffer to be transmitted.
 * @param length number of bytes to transmit.
 */
void drv_rs232_transmit(const uint8_t *data, uint32_t length);

/**
 * @brief get the number of available bytes in the rx ring buffer.
 *
 * @return number of unread bytes.
 */
uint32_t drv_rs232_available(void);

/**
 * @brief read a single byte from the rx ring buffer.
 *
 * @param byte pointer to store the received byte.
 * @return true if a byte was successfully read, false if the buffer is empty.
 */
bool drv_rs232_read(uint8_t *byte);

#endif /* DRV_RS232_H */