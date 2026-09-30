#include "drv_led.h"
#include <stddef.h>

/* Initialize the LED instance and link its HAL */
void drv_led_init(drv_led_t *led, const led_hal_interface_t *hal, void *hw_context) {
    if (led == NULL || hal == NULL) return;

    led->hal = hal;
    led->hw_context = hw_context;
    
    /* Default status */
    led->mode = LED_MODE_OFF;
    led->current_brightness = 0;
    led->max_brightness = 100;
    led->tick_counter = 0;
    led->interval_ms = 0;
    led->breathe_step = 5;

    /* Call hardware initialization if the function pointer is provided */
    if (led->hal->init != NULL) {
        led->hal->init(led->hw_context);
    }
    
    /* Ensure hardware output is off initially */
    if (led->hal->set_brightness != NULL) {
        led->hal->set_brightness(led->hw_context, 0);
    }
}

/* Turn off the LED completely */
void drv_led_set_off(drv_led_t *led) {
    if (led == NULL) return;
    
    led->mode = LED_MODE_OFF;
    led->current_brightness = 0;
    if (led->hal->set_brightness != NULL) {
        led->hal->set_brightness(led->hw_context, 0);
    }
}

/* Turn on the LED with a specific constant brightness */
void drv_led_set_on(drv_led_t *led, uint8_t brightness) {
    if (led == NULL) return;
    
    led->mode = LED_MODE_ON;
    if (brightness > led->max_brightness) {
        brightness = led->max_brightness;
    }
    led->current_brightness = brightness;
    
    if (led->hal->set_brightness != NULL) {
        led->hal->set_brightness(led->hw_context, led->current_brightness);
    }
}

/* Start blinking mode */
void drv_led_set_blink(drv_led_t *led, uint16_t interval_ms, uint8_t max_brightness) {
    if (led == NULL) return;
    
    /* Cap maximum brightness at 100% */
    if (max_brightness > 100) {
        max_brightness = 100;
    }
    
    led->mode = LED_MODE_BLINK;
    led->interval_ms = interval_ms;
    led->max_brightness = max_brightness; /* Set target blink brightness */
    
    led->tick_counter = 0;
    led->current_brightness = 0; /* Start from OFF state for clean animation */
}

/* Start breathing mode */
void drv_led_set_breathe(drv_led_t *led, uint16_t interval_ms, uint8_t max_brightness) {
    if (led == NULL) return;

    /* Cap maximum brightness at 100% */
    if (max_brightness > 100) {
        max_brightness = 100;
    }
    
    led->mode = LED_MODE_BREATHE;
    led->interval_ms = interval_ms;
    led->max_brightness = max_brightness; /* Set peak breathing brightness */
    
    led->tick_counter = 0;
    led->current_brightness = 0;
    led->breathe_step = 5; /* Default brightness change per tick */
}

/* --- Getter Methods Implementation --- */

/* Get current LED working mode */
led_mode_e drv_led_get_current_mode(drv_led_t *led) {
    if (led == NULL) {
        return LED_MODE_OFF; /* Return default state if pointer is invalid */
    }
    return led->mode;
}

/* Get current LED brightness (0-100) */
uint8_t drv_led_get_current_brightness(drv_led_t *led) {
    if (led == NULL) {
        return 0; /* Return 0 brightness if pointer is invalid */
    }
    return led->current_brightness;
}

/* Core animation engine: Must be called in a low-frequency timer or task */
void drv_led_tick_handler(drv_led_t *led, uint16_t tick_period_ms) {
    if (led == NULL) return;
    
    if (led->mode == LED_MODE_OFF || led->mode == LED_MODE_ON) {
        return; /* No animation update required */
    }

    led->tick_counter += tick_period_ms;

    if (led->tick_counter >= led->interval_ms) {
        led->tick_counter = 0;

        switch (led->mode) {
            case LED_MODE_BLINK:
                /* Toggle brightness between 0 and max */
                if (led->current_brightness > 0) {
                    led->current_brightness = 0;
                } else {
                    led->current_brightness = led->max_brightness;
                }
                break;

            case LED_MODE_BREATHE:
                /* Increment or decrement brightness */
                led->current_brightness += led->breathe_step;
                
                /* Check upper bound */
                if (led->current_brightness >= led->max_brightness) {
                    led->current_brightness = led->max_brightness;
                    led->breathe_step = -led->breathe_step; /* Reverse to fade out */
                } 
                /* Check lower bound (handling potential underflow via > max check) */
                else if (led->current_brightness == 0 || led->current_brightness > led->max_brightness) {
                    led->current_brightness = 0;
                    led->breathe_step = -led->breathe_step; /* Reverse to fade in */
                }
                break;

            default:
                break;
        }

        /* Execute the polymorphism: update physical hardware */
        if (led->hal->set_brightness != NULL) {
            led->hal->set_brightness(led->hw_context, led->current_brightness);
        }
    }
}