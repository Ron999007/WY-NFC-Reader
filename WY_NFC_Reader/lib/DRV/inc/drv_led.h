/**
 * @file    drv_led.h
 * @brief   Hardware Abstraction Layer (HAL) based Object-Oriented LED Driver.
 * * @note    [Quick Start Guide]
 * 1. Define your hardware-specific HAL interface:
 * const led_hal_interface_t my_led_hal = {
 * .init = my_pwm_init,
 * .set_brightness = my_pwm_set_duty
 * };
 * 2. Create an LED instance:
 * drv_led_t led_red;
 * 3. Initialize the instance:
 * drv_led_init(&led_red, &my_led_hal, (void*)PWM_CH0);
 * 4. Call the tick handler in a periodic timer ISR (e.g., every 10ms):
 * drv_led_tick_handler(&led_red, 10);
 * 5. Control the LED in your application layer:
 * drv_led_set_breathe(&led_red, 50, 100);
 */

#ifndef DRV_LED_H
#define DRV_LED_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief LED working modes
 */
typedef enum {
    LED_MODE_OFF = 0,    /**< LED is turned off completely */
    LED_MODE_ON,         /**< LED is turned on at a constant brightness */
    LED_MODE_BLINK,      /**< LED alternates between 0 and max brightness */
    LED_MODE_BREATHE     /**< LED smoothly fades in and out */
} led_mode_e;

/**
 * @brief Hardware Abstraction Layer (HAL) interface for LED
 * @note  This structure maps the logical LED driver to physical hardware controls (like PWM).
 */
typedef struct {
    /** * @brief Pointer to the hardware initialization function. 
     * @param hw_context User-defined hardware context (e.g., Timer/PWM base address).
     */
    void (*init)(void *hw_context);
    
    /** * @brief Pointer to the hardware brightness control function.
     * @param hw_context User-defined hardware context.
     * @param brightness Target brightness percentage (0 to 100).
     */
    void (*set_brightness)(void *hw_context, uint8_t brightness); 
} led_hal_interface_t;

/**
 * @brief LED instance structure (Object-Oriented C approach)
 * @note  Users should NOT modify these variables directly. Use the provided API.
 */
typedef struct {
    /* --- Hardware binding --- */
    const led_hal_interface_t *hal; /*!< Pointer to the HAL interface implementation */
    void *hw_context;               /*!< Pointer to hardware-specific resource/data */
    
    /* --- Software status --- */
    led_mode_e mode;                /*!< Current LED operation mode */
    uint8_t current_brightness;     /*!< Current brightness percentage (0-100) */
    uint8_t max_brightness;         /*!< Maximum brightness limit configured for effects */
    
    /* --- Animation control variables --- */
    uint16_t tick_counter;          /*!< Internal time counter for animations */
    uint16_t interval_ms;           /*!< Target interval for state changes (ms) */
    int8_t breathe_step;            /*!< Internal increment/decrement step for breathing */
} drv_led_t;

/* ==============================================================================
 * Public API Prototypes
 * ============================================================================== */

/**
 * @brief Initialize the LED object and bind it to specific hardware.
 * @param led Pointer to the LED instance.
 * @param hal Pointer to the constant HAL interface structure.
 * @param hw_context Custom hardware context passed to the HAL functions (e.g., &PA5).
 */
void drv_led_init(drv_led_t *led, const led_hal_interface_t *hal, void *hw_context);

/**
 * @brief Turn off the LED completely (0% brightness).
 * @param led Pointer to the LED instance.
 */
void drv_led_set_off(drv_led_t *led);

/**
 * @brief Turn on the LED with a constant brightness.
 * @param led Pointer to the LED instance.
 * @param brightness Target brightness percentage (0 to 100).
 */
void drv_led_set_on(drv_led_t *led, uint8_t brightness);

/**
 * @brief Set the LED to blinking mode.
 * @param led Pointer to the LED instance.
 * @param interval_ms Time in milliseconds to stay ON or OFF.
 * @param max_brightness The brightness percentage when the LED is ON (0 to 100).
 */
void drv_led_set_blink(drv_led_t *led, uint16_t interval_ms, uint8_t max_brightness);

/**
 * @brief Set the LED to breathing mode (smooth fade in and out).
 * @param led Pointer to the LED instance.
 * @param interval_ms Time in milliseconds between each step of brightness change.
 * @param max_brightness The peak brightness percentage of the breath cycle (0 to 100).
 */
void drv_led_set_breathe(drv_led_t *led, uint16_t interval_ms, uint8_t max_brightness);

/* ==============================================================================
 * Getter Methods (Safe Data Access)
 * ============================================================================== */

/**
 * @brief Get the current operation mode of the LED.
 * @param led Pointer to the LED instance.
 * @return led_mode_e The current working mode (OFF, ON, BLINK, BREATHE).
 */
led_mode_e drv_led_get_current_mode(drv_led_t *led);

/**
 * @brief Get the actual physical brightness of the LED at this exact moment.
 * @param led Pointer to the LED instance.
 * @return uint8_t Current brightness percentage (0 to 100).
 */
uint8_t drv_led_get_current_brightness(drv_led_t *led);

/* ==============================================================================
 * Core Animation Engine
 * ============================================================================== */

/**
 * @brief The heart of the LED animations. Drives the state machine and hardware.
 * @note  MUST be called periodically in a timer ISR or a dedicated RTOS task.
 * @param led Pointer to the LED instance.
 * @param tick_period_ms The exact time interval in milliseconds since the last call 
 * (e.g., pass '10' if called in a 10ms SysTick).
 */
void drv_led_tick_handler(drv_led_t *led, uint16_t tick_period_ms);

#endif /* DRV_LED_H */