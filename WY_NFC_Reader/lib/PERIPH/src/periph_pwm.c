#include <stddef.h>
#include "macro_utils.h"
#include "periph_pwm.h"

/* * Hardware Mapping Table 
 * Maps generic PWM_CHANNEL_X to specific Nuvoton EPWM module and channel.
 */
typedef struct {
    EPWM_T *epwm_base;   /* Pointer to EPWM module (e.g., EPWM0, EPWM1) */
    uint32_t hw_channel; /* Hardware channel number (0 to 5) */
} pwm_hw_mapping_t;

static const pwm_hw_mapping_t g_pwm_map[PWM_CHANNEL_MAX] = {
    [PWM_CHANNEL_0] = {EPWM1, 0},
    [PWM_CHANNEL_1] = {EPWM1, 1},
    [PWM_CHANNEL_2] = {EPWM1, 2},
    [PWM_CHANNEL_3] = {EPWM1, 3},
    [PWM_CHANNEL_4] = {EPWM1, 4},
    [PWM_CHANNEL_5] = {EPWM1, 5},
    /* Add more mapping here based on your actual schematic/wiring */
};

/* Initialize the specific PWM channel */
void periph_pwm_init(pwm_channel_e channel, pwm_config_t *config) {
    if (channel >= PWM_CHANNEL_MAX || config == NULL) {
        return;
    }
    
    EPWM_T *epwm = g_pwm_map[channel].epwm_base;
    uint32_t ch = g_pwm_map[channel].hw_channel;
    
    /* * NOTE: Clock enable (CLK_EnableModuleClock) and 
     * GPIO Multi-function Pin (SYS->GPA_MFPL) setup 
     * should be handled in SYS_Init() or board initialization phase.
     */
    
    /* Use Nuvoton BSP API to configure frequency and initial duty cycle */
    EPWM_ConfigOutputChannel(epwm, ch, config->frequency_hz, config->duty_cycle);
}

/* Set the PWM duty cycle (0-100%) dynamically */
void periph_pwm_set_duty(pwm_channel_e channel, uint8_t duty_cycle) {
    if (channel >= PWM_CHANNEL_MAX) {
        return;
    }
    
    if (duty_cycle > 100) {
        duty_cycle = 100;
    }
    
    EPWM_T *epwm = g_pwm_map[channel].epwm_base;
    uint32_t ch = g_pwm_map[channel].hw_channel;
    
    /* * Fetch current period value (CNR) and calculate new comparator value (CMR).
     * Formula based on Nuvoton BSP: CMR = DutyCycle * (CNR + 1) / 100 
     */
    uint32_t u32CNR = EPWM_GET_CNR(epwm, ch);
    EPWM_SET_CMR(epwm, ch, (duty_cycle * (u32CNR + 1)) / 100);
}

/* Set the PWM frequency (Hz) dynamically */
void periph_pwm_set_frequency(pwm_channel_e channel, uint32_t frequency_hz) {
    if (channel >= PWM_CHANNEL_MAX || frequency_hz == 0) {
        return;
    }
    
    EPWM_T *epwm = g_pwm_map[channel].epwm_base;
    uint32_t ch = g_pwm_map[channel].hw_channel;
    
    /* Retrieve current duty cycle to maintain it across frequency changes */
    uint32_t u32CNR = EPWM_GET_CNR(epwm, ch);
    uint32_t u32CMR = EPWM_GET_CMR(epwm, ch);
    uint8_t current_duty = 0;
    
    if (u32CNR != 0) {
        current_duty = (uint8_t)((u32CMR * 100) / (u32CNR + 1));
    }
    
    /* Reconfigure channel with new frequency and existing duty cycle */
    EPWM_ConfigOutputChannel(epwm, ch, frequency_hz, current_duty);
}

/* Enable PWM output */
void periph_pwm_enable(pwm_channel_e channel) {
    if (channel >= PWM_CHANNEL_MAX) {
        return;
    }
    
    EPWM_T *epwm = g_pwm_map[channel].epwm_base;
    uint32_t ch = g_pwm_map[channel].hw_channel;
    uint32_t ch_mask = (1UL << ch);
    
    /* Enable output pin generation and start counter */
    EPWM_EnableOutput(epwm, ch_mask);
    EPWM_Start(epwm, ch_mask);
}

/* Disable PWM output */
void periph_pwm_disable(pwm_channel_e channel) {
    if (channel >= PWM_CHANNEL_MAX) {
        return;
    }
    
    EPWM_T *epwm = g_pwm_map[channel].epwm_base;
    uint32_t ch = g_pwm_map[channel].hw_channel;
    uint32_t ch_mask = (1UL << ch);
    
    /* Stop counter and disable output pin */
    EPWM_Stop(epwm, ch_mask);
    EPWM_DisableOutput(epwm, ch_mask);
}