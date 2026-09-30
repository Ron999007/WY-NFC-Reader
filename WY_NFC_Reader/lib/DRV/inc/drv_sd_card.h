/**
 * @file    drv_sd_card.h
 * @brief   Hardware Abstraction Layer (HAL) Interface for SD Card
 * @details This file defines the standard interface and API for SD card operations.
 * It decouples the application and FatFs from specific hardware implementations.
 */

#ifndef DRV_SD_CARD_H
#define DRV_SD_CARD_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * 1. Hardware Abstraction Interface (The Contract)
 * ==================================================================== */

/**
 * @brief  Hardware Abstraction Layer (HAL) structure for SD Card.
 * @details Any new MCU platform (e.g., STM32, Nuvoton) must implement these
 * function pointers and register the struct using drv_sd_register_hal().
 */
typedef struct {
    /**
     * @brief  Check if the SD card is physically inserted into the slot.
     * @return true if inserted, false if removed or not detected.
     */
    bool     (*is_inserted)(void);

    /**
     * @brief  Initialize and probe the SD hardware controller.
     * @return true if initialization is successful, false otherwise.
     */
    bool     (*init)(void);

    /**
     * @brief  Safely disable the SD hardware controller and its interrupts.
     */
    void     (*deinit)(void);

    /**
     * @brief  Perform a deep hardware reset to clear deadlocks and re-initialize.
     */
    void     (*reset)(void);
    
    /**
     * @brief  Read sector(s) from the SD card.
     * @param  buff   Pointer to the data buffer to store read data.
     * @param  sector Start sector number in Logical Block Addressing (LBA).
     * @param  count  Number of sectors to read.
     * @return true if read is successful, false if hardware error occurs.
     */
    bool     (*read_blocks)(uint8_t *buff, uint32_t sector, uint32_t count);

    /**
     * @brief  Write sector(s) to the SD card.
     * @param  buff   Pointer to the data to be written.
     * @param  sector Start sector number in Logical Block Addressing (LBA).
     * @param  count  Number of sectors to write.
     * @return true if write is successful, false if hardware error occurs.
     */
    bool     (*write_blocks)(const uint8_t *buff, uint32_t sector, uint32_t count);

    /**
     * @brief  Get the total number of sectors available on the SD card.
     * @return Total sector count (Capacity = sector_count * 512 bytes).
     */
    uint32_t (*get_sector_count)(void);
} sd_hal_driver_t;

/* ====================================================================
 * 2. Registration API
 * ==================================================================== */

/**
 * @brief  Register a specific hardware implementation to the SD driver core.
 * @note   This MUST be called in main() before initializing any SD tasks.
 * @param  hal_driver Pointer to the populated sd_hal_driver_t structure.
 */
void drv_sd_register_hal(const sd_hal_driver_t *hal_driver);

/* ====================================================================
 * 3. Generic Wrapper APIs (Called by task_sdcard.c and diskio.c)
 * ==================================================================== */

bool drv_sd_is_card_inserted(void);
bool drv_sd_hardware_init(void);
void drv_sd_hardware_deinit(void);
void drv_sd_hardware_reset(void);

bool drv_sd_read_blocks(uint8_t *buff, uint32_t sector, uint32_t count);
bool drv_sd_write_blocks(const uint8_t *buff, uint32_t sector, uint32_t count);
uint32_t drv_sd_get_sector_count(void);

#endif /* DRV_SD_CARD_H */