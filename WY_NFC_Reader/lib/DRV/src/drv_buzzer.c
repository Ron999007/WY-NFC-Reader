#include "drv_buzzer.h"
#include <stddef.h>

static const buzzer_hardware_t *g_hw_ops = NULL;

void drv_buzzer_init(const buzzer_hardware_t *hw_ops) {
    if (hw_ops != NULL) {
        g_hw_ops = hw_ops;
        if (g_hw_ops->init != NULL) {
            g_hw_ops->init();
        }
    }
}

void drv_buzzer_set_voice(uint32_t freq) {
    if (g_hw_ops != NULL && g_hw_ops->set_frequency != NULL) {
        g_hw_ops->set_frequency(freq);
    }
}