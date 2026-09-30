#ifndef TASK_RGB_LED_H
#define TASK_RGB_LED_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * System Task API
 * ==================================================================== */

/**
 * @brief Initialize the RGB LED module and hardware.
 */
void Task_RGB_LED_Init(void);

/**
 * @brief Main execution loop for the RGB LED module.
 * MUST be called continuously in the main super-loop.
 */
void Task_RGB_LED(void);

/* ====================================================================
 * Public Control API (Can be called by other tasks)
 * ==================================================================== */

/**
 * @brief Send a command to set a static color.
 */
void Task_RGB_SetStaticColor(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Send a command to start the 7-color jump animation.
 */
void Task_RGB_Start7ColorJump(uint16_t interval_ms);

/**
 * @brief Send a command to start the rainbow cycle animation.
 */
void Task_RGB_StartRainbowCycle(float hue_step, uint16_t interval_ms);

/**
 * @brief Send a command to start the breathing animation.
 */
void Task_RGB_StartBreathing(float start_hue, float hue_step, float max_brightness, float brightness_step, uint16_t interval_ms);

/**
 * @brief Send a command to start the sparkle animation.
 */
void Task_RGB_StartSparkle(uint16_t interval_ms);

/**
 * @brief Send a command to start a custom color blink animation.
 * @param r Red value (0-100)
 * @param g Green value (0-100)
 * @param b Blue value (0-100)
 * @param interval_ms Blink interval in milliseconds (e.g., 500 for 1Hz)
 */
void Task_RGB_StartBlink(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms);

#endif /* TASK_RGB_LED_H */