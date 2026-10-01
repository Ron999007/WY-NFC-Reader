#include "MyApplication.h"

/* ====================================================================
 * Private Hardware Context & HAL Implementation
 * ==================================================================== */
typedef struct {
    void (*set_pin_state)(uint8_t state); 
    uint8_t target_brightness;            
    uint8_t soft_pwm_counter;             
} hw_soft_pwm_ctx_t;

/* Board-specific GPIO control functions */
static void board_set_led_r_pin(uint8_t state) { PB7 = (state > 0) ? 1 : 0; }
static void board_set_led_g_pin(uint8_t state) { PB8 = (state > 0) ? 1 : 0; }

static hw_soft_pwm_ctx_t ctx_led_r = { .set_pin_state = board_set_led_r_pin };
static hw_soft_pwm_ctx_t ctx_led_g = { .set_pin_state = board_set_led_g_pin };

static void hal_soft_pwm_init(void *hw_context) {
    hw_soft_pwm_ctx_t *ctx = (hw_soft_pwm_ctx_t *)hw_context;
    ctx->target_brightness = 0;
    ctx->soft_pwm_counter = 0;
    if (ctx->set_pin_state != NULL) ctx->set_pin_state(0); 
}

static void hal_soft_pwm_set_brightness(void *hw_context, uint8_t brightness) {
    hw_soft_pwm_ctx_t *ctx = (hw_soft_pwm_ctx_t *)hw_context;
    ctx->target_brightness = brightness; 
}

const led_hal_interface_t HAL_SW_PWM = {
    .init = hal_soft_pwm_init,
    .set_brightness = hal_soft_pwm_set_brightness
};

/* ====================================================================
 * Public Global Variables
 * ==================================================================== */
drv_led_t g_leds[LED_ID_MAX];

/* ====================================================================
 * Private Configuration Table
 * ==================================================================== */
typedef struct {
    const led_hal_interface_t *hal;
    void *hw_context;
} led_config_t;

static const led_config_t g_led_config_table[LED_ID_MAX] = {
    [LED_ID_R] = { .hal = &HAL_SW_PWM, .hw_context = &ctx_led_r },
    [LED_ID_G] = { .hal = &HAL_SW_PWM, .hw_context = &ctx_led_g }
};

/* ====================================================================
 * Task API Implementation
 * ==================================================================== */
void task_led_init(void)
{
    /* Set PB7(LED_R), PB8(LED_G) as LED */
    SYS->GPB_MFPL &= ~SYS_GPB_MFPL_PB7MFP_Msk;
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB7MFP_GPIO;
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB8MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB8MFP_GPIO);
    GPIO_SetMode(PB, (BIT7 | BIT8), GPIO_MODE_OUTPUT);
    LED_R = 0;
    LED_G = 0;
    
    /* 1. Initialize all LEDs */
    for (int i = 0; i < LED_ID_MAX; i++) {
        drv_led_init(&g_leds[i], g_led_config_table[i].hal, g_led_config_table[i].hw_context);
    }

    /* 2. Start TIMER0 for soft PWM */
    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 10000);
    TIMER_EnableInt(TIMER0);
    NVIC_EnableIRQ(TMR0_IRQn);
    TIMER_Start(TIMER0);
    
    /* 3. Set default states */
    //drv_led_set_breathe(&g_leds[LED_ID_R], 50, 100);
    //drv_led_set_on(&g_leds[LED_ID_G], 100);
    //drv_led_set_blink(&g_leds[LED_ID_G], 100, 100);
}

void task_led(void)
{
    static uint32_t u32_TickCnt_Temp = 0;
       
    if(Get_TickCount() - u32_TickCnt_Temp >= 10)
    {
        for (int i = 0; i < LED_ID_MAX; i++) {
            drv_led_tick_handler(&g_leds[i], 10); 
        }
        u32_TickCnt_Temp += 10;
    }
}

/* ====================================================================
 * Interrupt Service Routines (Hidden from Main App)
 * ==================================================================== */
void TMR0_IRQHandler(void)
{
    hw_soft_pwm_ctx_t *sw_led_contexts[] = { &ctx_led_r, &ctx_led_g };
    uint8_t num_sw_leds = sizeof(sw_led_contexts) / sizeof(sw_led_contexts[0]);

    for (int i = 0; i < num_sw_leds; i++) {
        hw_soft_pwm_ctx_t *ctx = sw_led_contexts[i];
        
        ctx->soft_pwm_counter++;
        if (ctx->soft_pwm_counter >= 100) {
            ctx->soft_pwm_counter = 0;
        }

        if (ctx->set_pin_state != NULL) {
            if (ctx->soft_pwm_counter < ctx->target_brightness) {
                ctx->set_pin_state(1); /* Output High */
            } else {
                ctx->set_pin_state(0); /* Output Low */
            }
        }
    }

    /* Clear timer interrupt flag */
    TIMER_ClearIntFlag(TIMER0);
}