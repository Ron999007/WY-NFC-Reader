/**
 * @file    drv_eeprom.c
 * @brief   AT24C32 EEPROM Device Driver Implementation
 */
#include "drv_eeprom.h"
#include <stddef.h>

/**
 * @brief Initializes the EEPROM driver handle with the provided I2C interface.
 */
void drv_eeprom_init(drv_eeprom_handle_t *handle, i2c_io_interface_t *i2c_io) {
    if (handle == NULL || i2c_io == NULL) return;
    
    handle->i2c_io = i2c_io;
    handle->state = DRV_EEPROM_STATE_IDLE;
    handle->is_busy = false;
    handle->last_status = DRV_EEPROM_STATUS_OK;
}

/**
 * @brief Executes the EEPROM state machine. Must be called periodically.
 */
void drv_eeprom_run(drv_eeprom_handle_t *handle) {
    if (handle == NULL || handle->i2c_io == NULL) return;

    switch (handle->state) {
        case DRV_EEPROM_STATE_IDLE:
            /* Waiting for new operation requests */
            break;

        case DRV_EEPROM_STATE_WRITE_START: {
            if (handle->remaining_len == 0) {
                handle->is_busy = false;
                handle->state = DRV_EEPROM_STATE_IDLE;
                break;
            }

            /* Calculate the maximum bytes that can be written in the current page */
            uint16_t page_offset = handle->target_addr % DRV_EEPROM_PAGE_SIZE;
            uint32_t bytes_to_write = DRV_EEPROM_PAGE_SIZE - page_offset;
            
            if (bytes_to_write > handle->remaining_len) {
                bytes_to_write = handle->remaining_len;
            }

            /* Execute hardware I2C write */
            bool success = handle->i2c_io->write_reg(
                handle->i2c_io->user_data, 
                DRV_EEPROM_I2C_ADDR, 
                handle->target_addr, 
                handle->data_ptr, 
                bytes_to_write
            );

            if (success) {
                /* Advance pointers and decrease remaining length */
                handle->target_addr += bytes_to_write;
                handle->data_ptr += bytes_to_write;
                handle->remaining_len -= bytes_to_write;
                
                /* Record current tick and transition to wait state */
                handle->wait_start_tick = handle->i2c_io->get_tick();
                handle->state = DRV_EEPROM_STATE_WRITE_WAIT;
            } else {
                handle->last_status = DRV_EEPROM_STATUS_ERROR_I2C;
                handle->state = DRV_EEPROM_STATE_ERROR;
            }
            break;
        }

        case DRV_EEPROM_STATE_WRITE_WAIT: {
            /* Check if the internal write cycle delay (5ms) has elapsed */
            uint32_t current_tick = handle->i2c_io->get_tick();
            if ((current_tick - handle->wait_start_tick) >= DRV_EEPROM_WRITE_TIME_MS) {
                if (handle->remaining_len > 0) {
                    /* Continue writing the next page */
                    handle->state = DRV_EEPROM_STATE_WRITE_START;
                } else {
                    /* Write operation fully completed */
                    handle->is_busy = false;
                    handle->last_status = DRV_EEPROM_STATUS_OK;
                    handle->state = DRV_EEPROM_STATE_IDLE;
                }
            }
            break;
        }

        case DRV_EEPROM_STATE_READ_START: {
            /* Sequential read does not require page boundary management */
            bool success = handle->i2c_io->read_reg(
                handle->i2c_io->user_data, 
                DRV_EEPROM_I2C_ADDR, 
                handle->target_addr, 
                handle->data_ptr, 
                handle->remaining_len
            );

            if (success) {
                handle->last_status = DRV_EEPROM_STATUS_OK;
            } else {
                handle->last_status = DRV_EEPROM_STATUS_ERROR_I2C;
            }
            
            handle->is_busy = false;
            handle->state = DRV_EEPROM_STATE_IDLE;
            break;
        }

        case DRV_EEPROM_STATE_ERROR:
            /* Reset busy flag and return to idle state after an error occurs */
            handle->is_busy = false;
            handle->state = DRV_EEPROM_STATE_IDLE;
            break;

        default:
            handle->state = DRV_EEPROM_STATE_IDLE;
            break;
    }
}

/**
 * @brief Initiates an asynchronous read operation from the EEPROM.
 */
drv_eeprom_status_t drv_eeprom_read_req(drv_eeprom_handle_t *handle, uint16_t addr, uint8_t *data, uint32_t len) {
    if (handle->is_busy) return DRV_EEPROM_STATUS_BUSY;
    if ((addr + len) > DRV_EEPROM_TOTAL_SIZE) return DRV_EEPROM_STATUS_ERROR_ADDR;

    handle->target_addr = addr;
    handle->data_ptr = data;
    handle->remaining_len = len;
    handle->is_busy = true;
    handle->state = DRV_EEPROM_STATE_READ_START;

    return DRV_EEPROM_STATUS_OK;
}

/**
 * @brief Initiates an asynchronous write operation to the EEPROM.
 */
drv_eeprom_status_t drv_eeprom_write_req(drv_eeprom_handle_t *handle, uint16_t addr, uint8_t *data, uint32_t len) {
    if (handle->is_busy) return DRV_EEPROM_STATUS_BUSY;
    if ((addr + len) > DRV_EEPROM_TOTAL_SIZE) return DRV_EEPROM_STATUS_ERROR_ADDR;

    handle->target_addr = addr;
    handle->data_ptr = data;
    handle->remaining_len = len;
    handle->is_busy = true;
    handle->state = DRV_EEPROM_STATE_WRITE_START;

    return DRV_EEPROM_STATUS_OK;
}

/**
 * @brief Checks if the EEPROM driver is currently busy processing a request.
 */
bool drv_eeprom_is_busy(drv_eeprom_handle_t *handle) {
    return handle->is_busy;
}