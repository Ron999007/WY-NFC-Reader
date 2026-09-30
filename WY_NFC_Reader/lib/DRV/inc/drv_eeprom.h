/**
 * @file    drv_eeprom.h
 * @brief   AT24C32 EEPROM Device Driver (Non-blocking State Machine)
 * @details Platform-independent driver layer. Requires an external I2C IO interface.
 */
#ifndef __DRV_EEPROM_H__
#define __DRV_EEPROM_H__

#include <stdint.h>
#include <stdbool.h>
#include "i2c_io.h"

/* AT24C32 Hardware Specifications */
#define DRV_EEPROM_I2C_ADDR       0x50  /* Default 7-bit I2C address */
#define DRV_EEPROM_PAGE_SIZE      32    /* 32 bytes per page */
#define DRV_EEPROM_TOTAL_SIZE     4096  /* 4KB total capacity */
#define DRV_EEPROM_WRITE_TIME_MS  5     /* Internal write cycle time in ms */

/**
 * @brief Enumeration of EEPROM internal state machine states.
 */
typedef enum {
    DRV_EEPROM_STATE_IDLE = 0,
    DRV_EEPROM_STATE_WRITE_START,
    DRV_EEPROM_STATE_WRITE_WAIT,
    DRV_EEPROM_STATE_READ_START,
    DRV_EEPROM_STATE_ERROR
} drv_eeprom_state_t;

/**
 * @brief Enumeration of EEPROM operation return statuses.
 */
typedef enum {
    DRV_EEPROM_STATUS_OK = 0,
    DRV_EEPROM_STATUS_BUSY,
    DRV_EEPROM_STATUS_ERROR_ADDR,
    DRV_EEPROM_STATUS_ERROR_I2C
} drv_eeprom_status_t;

/**
 * @brief EEPROM Driver Handle Structure.
 * @details Stores the current context, hardware interface, and operation variables.
 */
typedef struct {
    i2c_io_interface_t  *i2c_io;         /* Abstract I2C IO interface pointer */
    drv_eeprom_state_t  state;           /* Current state of the state machine */
    
    /* Transaction context */
    uint16_t            target_addr;     /* 16-bit internal memory address */
    uint8_t             *data_ptr;       /* Pointer to the data buffer */
    uint32_t            remaining_len;   /* Remaining bytes to process */
    
    /* Non-blocking timing context */
    uint32_t            wait_start_tick; /* Tick record for write cycle delay */
    
    /* Status flags */
    bool                is_busy;         /* True if an operation is in progress */
    drv_eeprom_status_t last_status;     /* Result of the last operation */
} drv_eeprom_handle_t;

/**
 * @brief Initializes the EEPROM driver handle with the provided I2C interface.
 * @param handle Pointer to the EEPROM driver handle structure.
 * @param i2c_io Pointer to the initialized I2C IO interface instance.
 */
void drv_eeprom_init(drv_eeprom_handle_t *handle, i2c_io_interface_t *i2c_io);

/**
 * @brief Executes the EEPROM state machine. Must be called periodically.
 * @param handle Pointer to the EEPROM driver handle structure.
 */
void drv_eeprom_run(drv_eeprom_handle_t *handle);

/**
 * @brief Initiates an asynchronous read operation from the EEPROM.
 * @param handle Pointer to the EEPROM driver handle structure.
 * @param addr   16-bit internal EEPROM address to read from.
 * @param data   Pointer to the buffer where read data will be stored.
 * @param len    Number of bytes to read.
 * @return DRV_EEPROM_STATUS_OK if the request is accepted, error code otherwise.
 */
drv_eeprom_status_t drv_eeprom_read_req(drv_eeprom_handle_t *handle, uint16_t addr, uint8_t *data, uint32_t len);

/**
 * @brief Initiates an asynchronous write operation to the EEPROM.
 * @param handle Pointer to the EEPROM driver handle structure.
 * @param addr   16-bit internal EEPROM address to write to.
 * @param data   Pointer to the data buffer to be written.
 * @param len    Number of bytes to write.
 * @return DRV_EEPROM_STATUS_OK if the request is accepted, error code otherwise.
 */
drv_eeprom_status_t drv_eeprom_write_req(drv_eeprom_handle_t *handle, uint16_t addr, uint8_t *data, uint32_t len);

/**
 * @brief Checks if the EEPROM driver is currently busy processing a request.
 * @param handle Pointer to the EEPROM driver handle structure.
 * @return true if the driver is busy, false if it is idle.
 */
bool drv_eeprom_is_busy(drv_eeprom_handle_t *handle);

#endif /* __DRV_EEPROM_H__ */