#ifndef PERIPH_PWM_H
#define PERIPH_PWM_H

#include <stdint.h>
#include <stdbool.h>

/* Define available PWM channels */
typedef enum {
    PWM_CHANNEL_0 = 0,
    PWM_CHANNEL_1,
    PWM_CHANNEL_2,
    PWM_CHANNEL_3,
    PWM_CHANNEL_4,
    PWM_CHANNEL_5,
    /* Add more channels as needed for your specific MCU */
    PWM_CHANNEL_MAX
} pwm_channel_e;

/* PWM configuration structure */
typedef struct {
    uint32_t frequency_hz; /* PWM frequency in Hertz */
    uint8_t duty_cycle;    /* Duty cycle percentage (0-100) */
} pwm_config_t;

/* Initialize the specific PWM channel */
void periph_pwm_init(pwm_channel_e channel, pwm_config_t *config);

/* Set the PWM duty cycle (0-100%) */
void periph_pwm_set_duty(pwm_channel_e channel, uint8_t duty_cycle);

/* Set the PWM frequency (Hz) */
void periph_pwm_set_frequency(pwm_channel_e channel, uint32_t frequency_hz);

/* Enable PWM output */
void periph_pwm_enable(pwm_channel_e channel);

/* Disable PWM output */
void periph_pwm_disable(pwm_channel_e channel);

#endif /* PERIPH_PWM_H */