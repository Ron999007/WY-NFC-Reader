#ifndef __TASK_LED_H__
#define __TASK_LED_H__

#include "macro_utils.h"
#include "drv_led.h"

/* Expose LED IDs so other tasks can reference them if needed */
typedef enum {
    LED_ID_R = 0, /* Red LED */
    LED_ID_G,     /* Green LED */
    LED_ID_MAX
} system_led_id_e;

/* Expose the global LED array if other tasks need to control them */
extern drv_led_t g_leds[LED_ID_MAX];

/* Public API for the LED Task */
void task_led_init(void);
void task_led(void);

#endif /* __TASK_LED_H__ */