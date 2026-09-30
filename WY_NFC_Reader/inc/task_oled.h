/**
 * @file    task_oled.h
 * @brief   Application Task for OLED UI Management Header.
 */

#ifndef __TASK_OLED_H__
#define __TASK_OLED_H__

#include <stdint.h>

/**
 * @brief Initializes the OLED task, configures the I2C peripheral, and sets the initial state.
 */
void Task_OLED_Init(void);

/**
 * @brief The main non-blocking polling function for the OLED task.
 */
void Task_OLED(void);

/**
 * @brief Sets the OLED to display a static, perfectly centered text on the dashboard.
 * @param text Pointer to a null-terminated string to display.
 */
void Task_OLED_Set_Dashboard(const char *text);

/**
 * @brief Sets the OLED to display a smooth, horizontally scrolling text (Marquee effect).
 * @param text     Pointer to a null-terminated string to scroll.
 * @param speed_ms The delay in milliseconds between each scroll frame.
 */
void Task_OLED_Set_Marquee(const char *text, uint32_t speed_ms);

/**
 * @brief Sets the OLED to reveal text character by character in a continuous loop.
 * @param text     Pointer to a null-terminated string to reveal.
 * @param speed_ms Delay in milliseconds before revealing the next character.
 * @param pause_ms Delay in milliseconds to pause after the full string is revealed before restarting.
 */
void Task_OLED_Set_Typewriter(const char *text, uint32_t speed_ms, uint32_t pause_ms);

/**
 * @brief Updates the dynamic network signal strength icon displayed on the top-left.
 * @param level The signal strength level ranging from 0 (No Signal) to 4 (Full Signal).
 */
void Task_OLED_Set_Signal_Level(uint8_t level);

/**
 * @brief Updates the dynamic battery capacity icon displayed on the top-right.
 * @param level The battery level ranging from 0 (Empty) to 3 (Full).
 */
void Task_OLED_Set_Battery_Level(uint8_t level);

#endif /* __TASK_OLED_H__ */