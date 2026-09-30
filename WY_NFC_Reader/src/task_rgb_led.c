#include "MyApplication.h"
#include "task_rgb_led.h"

/* ====================================================================
 * Mailbox Definitions (Hidden inside .c for Encapsulation)
 * ==================================================================== */

/* Define the command types for the RGB Mailbox */
typedef enum {
    RGB_CMD_NONE = 0,
    RGB_CMD_SET_STATIC,
    RGB_CMD_START_7COLOR,
    RGB_CMD_START_RAINBOW,
    RGB_CMD_START_BREATHING,
    RGB_CMD_START_SPARKLE,
    RGB_CMD_START_BLINK    /* Added Blink Command */
} rgb_cmd_e;

/* Define the message structure to hold command parameters */
typedef struct {
    rgb_cmd_e cmd;
    
    union {
        struct { uint8_t r; uint8_t g; uint8_t b; } static_color;
        struct { uint16_t interval_ms; } simple_anim;
        struct { float hue_step; uint16_t interval_ms; } rainbow;
        struct { float start_hue; float hue_step; float max_brightness; float brightness_step; uint16_t interval_ms; } breathing;
        struct { uint8_t r; uint8_t g; uint8_t b; uint16_t interval_ms; } blink; /* Added Blink Parameters */
    } params;
} rgb_msg_t;

/* Mailbox variables for bare-metal asynchronous communication */
static volatile bool s_has_new_cmd = false;
static rgb_msg_t s_mailbox;

/* ====================================================================
 * Private Hardware Context & HAL Implementation
 * ==================================================================== */

/* Context structure to hold the specific PWM channel for each LED */
typedef struct {
    pwm_channel_e channel; 
} hw_pwm_ctx_t;

/* Instantiate hardware contexts mapping to specific PWM channels */
static hw_pwm_ctx_t ctx_rgb_led_r = { .channel = PWM_CHANNEL_2 };
static hw_pwm_ctx_t ctx_rgb_led_g = { .channel = PWM_CHANNEL_1 };
static hw_pwm_ctx_t ctx_rgb_led_b = { .channel = PWM_CHANNEL_0 };

/* Initialize the hardware PWM channel */
static void hal_hw_pwm_init(void *hw_context) {
    hw_pwm_ctx_t *ctx = (hw_pwm_ctx_t *)hw_context;
    
    /* Configure PWM frequency to 1000Hz (1kHz is ideal for LEDs) with 0% duty */
    pwm_config_t cfg = {
        .frequency_hz = 1000,
        .duty_cycle = 0
    };
    
    periph_pwm_init(ctx->channel, &cfg);
    periph_pwm_enable(ctx->channel);
}

/* Set the duty cycle (brightness) of the hardware PWM channel */
static void hal_hw_pwm_set_brightness(void *hw_context, uint8_t brightness) {
    hw_pwm_ctx_t *ctx = (hw_pwm_ctx_t *)hw_context;
    periph_pwm_set_duty(ctx->channel, brightness);
}

/* The HAL interface structure for Hardware PWM */
static const led_hal_interface_t HAL_HW_PWM = {
    .init = hal_hw_pwm_init,
    .set_brightness = hal_hw_pwm_set_brightness
};

/* ====================================================================
 * Private Instances
 * ==================================================================== */

/* Base LED objects for the three color channels */
static drv_led_t g_led_red;
static drv_led_t g_led_green;
static drv_led_t g_led_blue;

/* Composite RGB object for color mixing and animations */
static drv_rgb_t g_my_rgb_light;

/* ====================================================================
 * Task API Implementation
 * ==================================================================== */

void Task_RGB_LED_Init(void)
{
    /* Initialize the base LEDs, linking them to the Hardware PWM HAL */
    drv_led_init(&g_led_red,   &HAL_HW_PWM, &ctx_rgb_led_r);
    drv_led_init(&g_led_green, &HAL_HW_PWM, &ctx_rgb_led_g);
    drv_led_init(&g_led_blue,  &HAL_HW_PWM, &ctx_rgb_led_b);

    /* Initialize the composite RGB object using the 3 base LEDs */
    drv_rgb_init(&g_my_rgb_light, &g_led_red, &g_led_green, &g_led_blue);

    /* Clear the mailbox initially */
    s_has_new_cmd = false;
    s_mailbox.cmd = RGB_CMD_NONE;

    /* Application Logic: Set default startup animation */
    //drv_rgb_start_breathing(&g_my_rgb_light, 0.0f, 60.0f, 1.0f, 0.02f, 20);
    
    drv_rgb_start_rainbow_cycle(&g_my_rgb_light, 1.5f, 20);
}

void Task_RGB_LED(void)
{
    static uint32_t u32_TickCnt_Temp = 0;

    /* 1. Check if a new command has been received from other tasks via the mailbox */
    if (s_has_new_cmd) {
        switch (s_mailbox.cmd) {
            case RGB_CMD_SET_STATIC:
                drv_rgb_set_color(&g_my_rgb_light, 
                                  s_mailbox.params.static_color.r, 
                                  s_mailbox.params.static_color.g, 
                                  s_mailbox.params.static_color.b);
                break;

            case RGB_CMD_START_7COLOR:
                drv_rgb_start_7color_jump(&g_my_rgb_light, 
                                          s_mailbox.params.simple_anim.interval_ms);
                break;

            case RGB_CMD_START_RAINBOW:
                drv_rgb_start_rainbow_cycle(&g_my_rgb_light, 
                                            s_mailbox.params.rainbow.hue_step, 
                                            s_mailbox.params.rainbow.interval_ms);
                break;

            case RGB_CMD_START_BREATHING:
                drv_rgb_start_breathing(&g_my_rgb_light, 
                                        s_mailbox.params.breathing.start_hue, 
                                        s_mailbox.params.breathing.hue_step, 
                                        s_mailbox.params.breathing.max_brightness, 
                                        s_mailbox.params.breathing.brightness_step, 
                                        s_mailbox.params.breathing.interval_ms);
                break;

            case RGB_CMD_START_SPARKLE:
                drv_rgb_start_sparkle(&g_my_rgb_light, 
                                      s_mailbox.params.simple_anim.interval_ms);
                break;

            case RGB_CMD_START_BLINK:
                drv_rgb_start_blink(&g_my_rgb_light, 
                                    s_mailbox.params.blink.r, 
                                    s_mailbox.params.blink.g, 
                                    s_mailbox.params.blink.b, 
                                    s_mailbox.params.blink.interval_ms);
                break;

            default:
                break;
        }
        
        /* Clear the flag after processing the command */
        s_has_new_cmd = false;
    }

    /* 2. Process the background animation periodically (e.g., every 10ms) */
    if (Get_TickCount() - u32_TickCnt_Temp >= 10)
    {
        drv_rgb_tick_handler(&g_my_rgb_light, 10);
        u32_TickCnt_Temp += 10;
    }
}

/* ====================================================================
 * Public Control API Implementations (Writers to the Mailbox)
 * ==================================================================== */

void Task_RGB_SetStaticColor(uint8_t r, uint8_t g, uint8_t b) {
    s_mailbox.params.static_color.r = r;
    s_mailbox.params.static_color.g = g;
    s_mailbox.params.static_color.b = b;
    s_mailbox.cmd = RGB_CMD_SET_STATIC;
    s_has_new_cmd = true; /* Raise flag to notify Task_RGB_LED */
}

void Task_RGB_Start7ColorJump(uint16_t interval_ms) {
    s_mailbox.params.simple_anim.interval_ms = interval_ms;
    s_mailbox.cmd = RGB_CMD_START_7COLOR;
    s_has_new_cmd = true;
}

void Task_RGB_StartRainbowCycle(float hue_step, uint16_t interval_ms) {
    s_mailbox.params.rainbow.hue_step = hue_step;
    s_mailbox.params.rainbow.interval_ms = interval_ms;
    s_mailbox.cmd = RGB_CMD_START_RAINBOW;
    s_has_new_cmd = true;
}

void Task_RGB_StartBreathing(float start_hue, float hue_step, float max_brightness, float brightness_step, uint16_t interval_ms) {
    s_mailbox.params.breathing.start_hue = start_hue;
    s_mailbox.params.breathing.hue_step = hue_step;
    s_mailbox.params.breathing.max_brightness = max_brightness;
    s_mailbox.params.breathing.brightness_step = brightness_step;
    s_mailbox.params.breathing.interval_ms = interval_ms;
    s_mailbox.cmd = RGB_CMD_START_BREATHING;
    s_has_new_cmd = true;
}

void Task_RGB_StartSparkle(uint16_t interval_ms) {
    s_mailbox.params.simple_anim.interval_ms = interval_ms;
    s_mailbox.cmd = RGB_CMD_START_SPARKLE;
    s_has_new_cmd = true;
}

void Task_RGB_StartBlink(uint8_t r, uint8_t g, uint8_t b, uint16_t interval_ms) {
    s_mailbox.params.blink.r = r;
    s_mailbox.params.blink.g = g;
    s_mailbox.params.blink.b = b;
    s_mailbox.params.blink.interval_ms = interval_ms;
    s_mailbox.cmd = RGB_CMD_START_BLINK;
    s_has_new_cmd = true;
}