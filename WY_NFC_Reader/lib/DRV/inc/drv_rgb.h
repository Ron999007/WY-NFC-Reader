#ifndef DRV_RGB_H
#define DRV_RGB_H

#include "drv_led.h"
#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * Definitions & Structures
 * ==================================================================== */

/**
 * @brief RGB working modes enumeration.
 * Represents the current animation state of the RGB composite LED.
 */
typedef enum {
    RGB_MODE_STATIC = 0,    /**< Solid color mode with no animation */
    RGB_MODE_7COLOR_JUMP,   /**< 7-color jumping animation mode */
    RGB_MODE_RAINBOW_CYCLE, /**< Smooth HSV rainbow cycle mode */
    RGB_MODE_BREATHING,     /**< Breathing animation with optional color changing */
    RGB_MODE_SPARKLE,       /**< Random color pop / sparkle mode */
    RGB_MODE_BLINK          /**< Custom color blinking mode */
} rgb_mode_e;

/**
 * @brief RGB Composite Object Structure.
 * @note Users should NOT modify these variables directly. Use the provided APIs.
 */
typedef struct {
    /* Pointers to the 3 underlying base LED objects */
    drv_led_t *led_r;
    drv_led_t *led_g;
    drv_led_t *led_b;

    /* RGB Status & Animation variables */
    rgb_mode_e mode;             /* Current working mode */
    uint8_t current_color_index; /* Index for the 7-color table (0 to 6) */
    uint16_t tick_counter;       /* Time counter for animations */
    uint16_t interval_ms;        /* Speed of color changing or animation */

    /* Variables for Rainbow Cycle, Breathing & Sparkle modes */
    float current_hue;           /* Current hue value (0.0 to 360.0) */
    float hue_step;              /* Hue step to advance per tick or per breath */

    /* Variables for Breathing mode */
    float current_brightness;    /* Current brightness level (0.0 to 1.0) */
    float brightness_step;       /* Brightness increment/decrement per tick */
    float max_brightness;        /* Maximum brightness limit (0.0 to 1.0) */
    bool is_breathing_in;        /* Flag: true = getting brighter, false = dimming */
    
    /* Variables for Blink mode */
    uint8_t blink_r;             /* Target Red value for blinking (0-100) */
    uint8_t blink_g;             /* Target Green value for blinking (0-100) */
    uint8_t blink_b;             /* Target Blue value for blinking (0-100) */
    bool is_blink_on;            /* Flag: true = LED is currently ON, false = OFF */
} drv_rgb_t;

/* ====================================================================
 * Public API Prototypes
 * ==================================================================== */

/**
 * @brief Initialize the RGB LED composite object.
 * @param rgb Pointer to the RGB object to initialize.
 * @param r Pointer to the initialized Red LED object.
 * @param g Pointer to the initialized Green LED object.
 * @param b Pointer to the initialized Blue LED object.
 */
void drv_rgb_init(drv_rgb_t *rgb, drv_led_t *r, drv_led_t *g, drv_led_t *b);

/**
 * @brief Set the RGB LED to a specific static color.
 * @param rgb   Pointer to the RGB object.
 * @param r_val Red brightness level (0 to 100).
 * @param g_val Green brightness level (0 to 100).
 * @param b_val Blue brightness level (0 to 100).
 */
void drv_rgb_set_color(drv_rgb_t *rgb, uint8_t r_val, uint8_t g_val, uint8_t b_val);

/**
 * @brief Start the 7-color jumping animation.
 * Cycles through Red, Green, Blue, Yellow, Cyan, Magenta, White.
 * @param rgb         Pointer to the RGB object.
 * @param interval_ms Time in milliseconds to hold each color before jumping to the next.
 */
void drv_rgb_start_7color_jump(drv_rgb_t *rgb, uint16_t interval_ms);

/**
 * @brief Start a smooth continuous rainbow cycle animation based on HSV color space.
 * @param rgb         Pointer to the RGB object.
 * @param hue_step    Degrees to advance the color hue per update tick (e.g., 1.5f for a smooth flow).
 * @param interval_ms Update interval in milliseconds (e.g., 20ms for 50FPS visual update).
 */
void drv_rgb_start_rainbow_cycle(drv_rgb_t *rgb, float hue_step, uint16_t interval_ms);

/**
 * @brief Start the breathing animation with optional automatic color shifting.
 * @param rgb             Pointer to the RGB object.
 * @param start_hue       The initial color hue in degrees (0.0 to 360.0, e.g., 0.0f = Red).
 * @param hue_step        Degrees to shift the hue after every complete breath cycle (in-out). 
 * Set to 0.0f to keep breathing the same color, or e.g., 60.0f for a 6-color cycle.
 * @param max_brightness  The peak brightness of the breath effect (0.0 to 1.0).
 * @param brightness_step Amount to increase/decrease the brightness ratio per update tick (e.g., 0.02f).
 * @param interval_ms     Update interval in milliseconds (e.g., 20ms).
 */
void drv_rgb_start_breathing(drv_rgb_t *rgb, float start_hue, float hue_step, float max_brightness, float brightness_step, uint16_t interval_ms);

/**
 * @brief Start a random color sparkle/pop animation.
 * @param rgb         Pointer to the RGB object.
 * @param interval_ms Time in milliseconds between each random color pop.
 */
void drv_rgb_start_sparkle(drv_rgb_t *rgb, uint16_t interval_ms);

/**
 * @brief Start a custom color blinking animation.
 * @param rgb         Pointer to the RGB object.
 * @param r_val       Target Red brightness level when ON (0 to 100).
 * @param g_val       Target Green brightness level when ON (0 to 100).
 * @param b_val       Target Blue brightness level when ON (0 to 100).
 * @param interval_ms Time in milliseconds to stay ON or OFF (e.g., 500 for a 1Hz blink rate).
 */
void drv_rgb_start_blink(drv_rgb_t *rgb, uint8_t r_val, uint8_t g_val, uint8_t b_val, uint16_t interval_ms);

/* ====================================================================
 * Background Animation Engine
 * ==================================================================== */

/**
 * @brief Background engine for driving RGB animations. 
 * @note  MUST be called periodically in a timer interrupt or a dedicated task loop.
 * @param rgb            Pointer to the RGB object.
 * @param tick_period_ms The exact time in milliseconds elapsed since the last call 
 * (e.g., pass '10' if called inside a 10ms periodic task).
 */
void drv_rgb_tick_handler(drv_rgb_t *rgb, uint16_t tick_period_ms);

#endif /* DRV_RGB_H */