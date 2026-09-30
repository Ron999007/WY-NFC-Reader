/**
 * @file    drv_sd_card.c
 * @brief   Platform Independent SD Driver Core
 * @details Routes generic API calls to the registered hardware-specific implementation.
 * This file contains ZERO hardware-specific code and never needs modification.
 */

#include "drv_sd_card.h"
#include <stddef.h>

/* Pointer to the currently registered hardware implementation */
static const sd_hal_driver_t *s_hal = NULL;

void drv_sd_register_hal(const sd_hal_driver_t *hal_driver) {
    s_hal = hal_driver;
}

bool drv_sd_is_card_inserted(void) {
    if (s_hal && s_hal->is_inserted) {
        return s_hal->is_inserted();
    }
    return false; 
}

bool drv_sd_hardware_init(void) {
    if (s_hal && s_hal->init) {
        return s_hal->init();
    }
    return false;
}

void drv_sd_hardware_deinit(void) {
    if (s_hal && s_hal->deinit) {
        s_hal->deinit();
    }
}

void drv_sd_hardware_reset(void) {
    if (s_hal && s_hal->reset) {
        s_hal->reset();
    }
}

bool drv_sd_read_blocks(uint8_t *buff, uint32_t sector, uint32_t count) {
    if (s_hal && s_hal->read_blocks) {
        return s_hal->read_blocks(buff, sector, count);
    }
    return false;
}

bool drv_sd_write_blocks(const uint8_t *buff, uint32_t sector, uint32_t count) {
    if (s_hal && s_hal->write_blocks) {
        return s_hal->write_blocks(buff, sector, count);
    }
    return false;
}

uint32_t drv_sd_get_sector_count(void) {
    if (s_hal && s_hal->get_sector_count) {
        return s_hal->get_sector_count();
    }
    return 0;
}