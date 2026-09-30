#include "drv_rgb.h"
#include <stddef.h>
#include <math.h>
#include <stdlib.h> /* Required for rand() in Sparkle mode */

/* Standard 7-color definition table (R, G, B) */
static const uint8_t RGB_7COLORS[7][3] = {
    {100,   0,   0}, /* 0: Red */
    {  0, 100,   0}, /* 1: Green */
    {  0,   0, 100}, /* 2: Blue */
    {100, 100,   0}, /* 3: Yellow */
    {  0, 100, 100}, /* 4: Cyan */
    {100,   0, 100}, /* 5: Magenta */
    {100, 100, 100}  /* 6: White */
};

/* Internal HSV to RGB conversion algorithm */
static void HSV_to_RGB(float h, float s, float v, uint8_t *r, uint8_t *g, uint8_t *b) {
    int i = (int)(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    float rf = 0, gf = 0, bf = 0;
    switch (i) {
        case 0: rf = v; gf = t; bf = p; break;
        case 1: rf = q; gf = v; bf = p; break;
        case 2: rf = p; gf = v; bf = t; break;
        case 3: rf = p; gf = q; bf = v; break;
        case 4: rf = t; gf = p; bf = v; break;
        case 5: rf = v; gf = p; bf = q; break;
    }
    
    *r = (uint8_t)(rf * 100.0f);
    *g = (uint8_t)(gf * 100.0f);
    *b = (uint8_t)(bf * 100.0f);
}

void drv_rgb_init(drv_rgb_t *rgb, drv_led_t *r, drv_led_t *g, drv_led_t *b) {
    if (rgb == NULL || r == NULL || g == NULL || b == NULL) {
        return;
    }

    rgb->led_r = r;
    rgb->led_g = g;
    rgb->led_b = b;
    rgb->mode = RGB_MODE_STATIC;
    rgb->current_color_index = 0;
    rgb->tick_counter = 0;
    rgb->interval_ms = 0;
    
    rgb->current_hue = 0.0f;
    rgb->hue_step = 0.0f;

    rgb->current_brightness = 0.0f;
    rgb->brightness_step = 0.0f;
    rgb->max_brightness = 1.0f;
    rgb->is_breathing_in = true;
    
    /* Initialize blink parameters */
    rgb->blink_r = 0;
    rgb->blink_g = 0;
    rgb->blink_b = 0;
    rgb->is_blink_on = false;

    drv_rgb_set_color(rgb, 0, 0, 0);
}

void drv_rgb_set_color(drv_rgb_t *rgb, uint8_t r_val, uint8_t g_val, uint8_t b_val) {
    if (rgb == NULL) return;
    
    rgb->mode = RGB_MODE_STATIC;
    drv_led_set_on(rgb->led_r, r_val);
    drv_led_set_on(rgb->led_g, g_val);
    drv_led_set_on(rgb->led_b, b_val);
}

void drv_rgb_start_7color_jump(drv_rgb_t *rgb, uint16_t interval_ms) {
    if (rgb == NULL) return;

    rgb->mode = RGB_MODE_7COLOR_JUMP;
    rgb->interval_ms = interval_ms;
    rgb->tick_counter = 0;
    rgb->current_color_index = 0;

    drv_led_set_on(rgb->led_r, RGB_7COLORS[0][0]);
    drv_led_set_on(rgb->led_g, RGB_7COLORS[0][1]);
    drv_led_set_on(rgb->led_b, RGB_7COLORS[0][2]);
}

void drv_rgb_start_rainbow_cycle(drv_rgb_t *rgb, float hue_step, uint16_t interval_ms) {
    if (rgb == NULL) return;
    
    rgb->mode = RGB_MODE_RAINBOW_CYCLE;
    rgb->interval_ms = interval_ms;
    rgb->tick_counter = 0;
    rgb->current_hue = 0.0f;  
    rgb->hue_step = hue_step; 
    
    uint8_t r, g, b;
    HSV_to_RGB(rgb->current_hue, 1.0f, 1.0f, &r, &g, &b);
    drv_led_set_on(rgb->led_r, r);
    drv_led_set_on(rgb->led_g, g);
    drv_led_set_on(rgb->led_b, b);
}

void drv_rgb_start_breathing(drv_rgb_t *rgb, float start_hue, float hue_step, float max_brightness, float brightness_step, uint16_t interval_ms) {
    if (rgb == NULL) return;
    
    rgb->mode = RGB_MODE_BREATHING;
    rgb->interval_ms = interval_ms;
    rgb->tick_counter = 0;
    
    rgb->current_hue = start_hue;            
    rgb->hue_step = hue_step;                
    
    if (max_brightness > 1.0f) rgb->max_brightness = 1.0f;
    else if (max_brightness < 0.0f) rgb->max_brightness = 0.0f;
    else rgb->max_brightness = max_brightness;

    rgb->brightness_step = brightness_step;  
    rgb->current_brightness = 0.0f;          
    rgb->is_breathing_in = true;             

    drv_led_set_on(rgb->led_r, 0);
    drv_led_set_on(rgb->led_g, 0);
    drv_led_set_on(rgb->led_b, 0);
}

void drv_rgb_start_sparkle(drv_rgb_t *rgb, uint16_t interval_ms) {
    if (rgb == NULL) return;
    
    rgb->mode = RGB_MODE_SPARKLE;
    rgb->interval_ms = interval_ms;
    rgb->tick_counter = 0;
    
    drv_led_set_on(rgb->led_r, 0);
    drv_led_set_on(rgb->led_g, 0);
    drv_led_set_on(rgb->led_b, 0);
}

/* New custom color blink animation */
void drv_rgb_start_blink(drv_rgb_t *rgb, uint8_t r_val, uint8_t g_val, uint8_t b_val, uint16_t interval_ms) {
    if (rgb == NULL) return;
    
    rgb->mode = RGB_MODE_BLINK;
    rgb->interval_ms = interval_ms;
    rgb->tick_counter = 0;
    
    /* Store the target color */
    rgb->blink_r = r_val;
    rgb->blink_g = g_val;
    rgb->blink_b = b_val;
    
    /* Start by turning the LED ON with the target color */
    rgb->is_blink_on = true;
    drv_led_set_on(rgb->led_r, rgb->blink_r);
    drv_led_set_on(rgb->led_g, rgb->blink_g);
    drv_led_set_on(rgb->led_b, rgb->blink_b);
}

void drv_rgb_tick_handler(drv_rgb_t *rgb, uint16_t tick_period_ms) {
    if (rgb == NULL || rgb->mode == RGB_MODE_STATIC) {
        return;
    }

    rgb->tick_counter += tick_period_ms;

    if (rgb->tick_counter >= rgb->interval_ms) {
        rgb->tick_counter = 0;

        uint8_t r = 0, g = 0, b = 0;

        switch (rgb->mode) {
            case RGB_MODE_7COLOR_JUMP:
                rgb->current_color_index++;
                if (rgb->current_color_index >= 7) {
                    rgb->current_color_index = 0; 
                }
                r = RGB_7COLORS[rgb->current_color_index][0];
                g = RGB_7COLORS[rgb->current_color_index][1];
                b = RGB_7COLORS[rgb->current_color_index][2];
                break;

            case RGB_MODE_RAINBOW_CYCLE:
                rgb->current_hue += rgb->hue_step;
                if (rgb->current_hue >= 360.0f) {
                    rgb->current_hue -= 360.0f;
                }
                HSV_to_RGB(rgb->current_hue, 1.0f, 1.0f, &r, &g, &b);
                break;

            case RGB_MODE_BREATHING:
                if (rgb->is_breathing_in) {
                    rgb->current_brightness += rgb->brightness_step;
                    if (rgb->current_brightness >= rgb->max_brightness) {
                        rgb->current_brightness = rgb->max_brightness;
                        rgb->is_breathing_in = false; 
                    }
                } else {
                    rgb->current_brightness -= rgb->brightness_step;
                    if (rgb->current_brightness <= 0.0f) {
                        rgb->current_brightness = 0.0f;
                        rgb->is_breathing_in = true; 
                        rgb->current_hue += rgb->hue_step;
                        if (rgb->current_hue >= 360.0f) {
                            rgb->current_hue -= 360.0f;
                        }
                    }
                }
                HSV_to_RGB(rgb->current_hue, 1.0f, rgb->current_brightness, &r, &g, &b);
                break;

            case RGB_MODE_SPARKLE:
                rgb->current_hue = (float)(rand() % 360);
                HSV_to_RGB(rgb->current_hue, 1.0f, 1.0f, &r, &g, &b);
                break;

            case RGB_MODE_BLINK:
                /* Toggle between ON and OFF */
                if (rgb->is_blink_on) {
                    r = 0; g = 0; b = 0;
                    rgb->is_blink_on = false;
                } else {
                    r = rgb->blink_r; g = rgb->blink_g; b = rgb->blink_b;
                    rgb->is_blink_on = true;
                }
                break;

            default:
                break;
        }

        /* Update the hardware outputs */
        drv_led_set_on(rgb->led_r, r);
        drv_led_set_on(rgb->led_g, g);
        drv_led_set_on(rgb->led_b, b);
    }
}