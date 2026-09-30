#ifndef DRV_BUZZER_H
#define DRV_BUZZER_H

#include <stdint.h>

/**
 * @brief Hardware abstraction interface for the buzzer.
 */
typedef struct {
    void (*init)(void);
    void (*set_frequency)(uint32_t freq); /* Set to 0 to mute */
} buzzer_hardware_t;

/**
 * @brief Initialize the buzzer driver with a hardware interface.
 * @param hw_ops Pointer to the hardware operations structure.
 */
void drv_buzzer_init(const buzzer_hardware_t *hw_ops);

/**
 * @brief Set the output frequency of the buzzer.
 * @param freq Target frequency in Hz.
 */
void drv_buzzer_set_voice(uint32_t freq);

#endif /* DRV_BUZZER_H */