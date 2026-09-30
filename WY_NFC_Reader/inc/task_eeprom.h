/**
 * @file    task_eeprom.h
 * @brief   Bare-metal Application Task Layer for EEPROM Management.
 * @details Provides a non-blocking queue and callback mechanism. 
 * Hardware interface is tightly bound internally.
 */
#ifndef __TASK_EEPROM_H__
#define __TASK_EEPROM_H__

#include <stdint.h>
#include <stdbool.h>

/* Callback function prototype for EEPROM operation completion */
typedef void (*eeprom_callback_t)(bool success, void *user_data);

/**
 * @brief Initializes the EEPROM task, internal I2C hardware, and ring buffer.
 * @note  No longer requires passing the IO interface pointer as it is bound internally.
 */
void task_eeprom_init(void);

/**
 * @brief Main execution task for EEPROM. 
 * @note  Renamed from task_eeprom_process. MUST be called periodically in the main super-loop.
 */
void task_eeprom(void);

/**
 * @brief Submits an asynchronous read request to the queue.
 */
bool task_eeprom_read(uint16_t addr, uint8_t *data, uint32_t len, eeprom_callback_t cb, void *user_data);

/**
 * @brief Submits an asynchronous write request to the queue.
 */
bool task_eeprom_write(uint16_t addr, const uint8_t *data, uint32_t len, eeprom_callback_t cb, void *user_data);

/**
 * @brief Checks if the internal request queue is full.
 */
bool task_eeprom_is_queue_full(void);

#endif /* __TASK_EEPROM_H__ */