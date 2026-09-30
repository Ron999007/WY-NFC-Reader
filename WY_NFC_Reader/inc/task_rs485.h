#ifndef TASK_RS485_H
#define TASK_RS485_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the RS485 application task layer.
 */
void task_rs485_init(void);

/**
 * @brief Process RS485 RX parsing and TX scheduling in the main super-loop.
 */
void task_rs485(void);

/**
 * @brief Schedule data to be sent over the RS485 bus.
 * * @param data Pointer to the data buffer.
 * @param length Number of bytes to send.
 * @return 0 on success, negative error code on failure.
 */
int8_t task_rs485_send(const uint8_t *data, uint16_t length);

/**
 * @brief Send a null-terminated string over the RS485 bus.
 *
 * @param str Pointer to the null-terminated string.
 * @return 0 on success, negative error code on failure.
 */
int8_t task_rs485_send_string(const char *str);

#endif /* TASK_RS485_H */