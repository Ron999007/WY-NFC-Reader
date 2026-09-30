#ifndef TASK_RS232_H
#define TASK_RS232_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief initialize the rs232 application task layer.
 */
void task_rs232_init(void);

/**
 * @brief process rs232 rx parsing and tx scheduling in the main super-loop.
 */
void task_rs232(void);

/**
 * @brief schedule data to be sent over the rs232 bus.
 *
 * @param data pointer to the data buffer.
 * @param length number of bytes to send.
 * @return 0 on success, negative error code on failure.
 */
int8_t task_rs232_send(const uint8_t *data, uint16_t length);

/**
 * @brief send a null-terminated string over the rs232 bus.
 *
 * @param str pointer to the null-terminated string.
 * @return 0 on success, negative error code on failure.
 */
int8_t task_rs232_send_string(const char *str);

#endif /* TASK_RS232_H */