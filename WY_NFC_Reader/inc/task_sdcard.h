/**
 * @file    task_sdcard.h
 * @brief   Non-blocking SD Card & FatFs Auto-Mount Task API
 * @details Provides high-level application interfaces for file system operations.
 */

#ifndef TASK_SDCARD_H
#define TASK_SDCARD_H

#include <stdbool.h>

/**
 * @brief  Initializes the SD card task variables, states, and registers HAL.
 * @note   Must be called once before the main super-loop starts.
 */
void Task_SDCard_Init(void);

/**
 * @brief  The main non-blocking routine for the SD card state machine.
 * @note   Must be called repeatedly within the main super-loop.
 */
void Task_SDCard(void);

/**
 * @brief  Checks if the SD card is successfully mounted and ready for file I/O.
 * @return true if the system is ready, false if the card is missing or mounting failed.
 */
bool Task_SDCard_is_ready(void);

/**
 * @brief  Appends a string message to a specified text file on the SD card.
 * @param  filename The target file path (e.g., "0:/sys_log.txt").
 * @param  message  The null-terminated string to append.
 * @return true if the string was successfully written, false if I/O failed.
 */
bool Task_SDCard_append_log(const char *filename, const char *message);

#endif /* TASK_SDCARD_H */