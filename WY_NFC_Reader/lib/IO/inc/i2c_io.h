/**
 * @file    i2c_io.h
 * @brief   Abstract I2C Communication Interface (The Contract).
 * @details Decouples device drivers (like OLED, EEPROM) from the specific MCU hardware.
 */
#ifndef __I2C_IO_H__
#define __I2C_IO_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Generic I2C IO Interface structure.
 * @details Drivers use this structure to interact with the underlying I2C bus.
 */
typedef struct {
    /** * @brief Opaque pointer to the hardware context (e.g., periph_i2c_obj_t). */
    void *user_data; 

    /** * @brief Writes data to a specific register address of the I2C slave.
     * @param user_data  Pointer to the hardware context.
     * @param slave_addr 7-bit I2C slave address.
     * @param reg_addr   The internal register address to write to (Control Byte for OLED).
     * @param data       Pointer to the data buffer to transmit.
     * @param len        Length of the data buffer.
     * @return true if successful, false otherwise.
     */
    bool (*write_reg)(void *user_data, uint8_t slave_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len);

    /** * @brief Reads data from a specific register address of the I2C slave.
     * @param user_data  Pointer to the hardware context.
     * @param slave_addr 7-bit I2C slave address.
     * @param reg_addr   The internal register address to read from.
     * @param data       Pointer to the buffer to store received data.
     * @param len        Number of bytes to read.
     * @return true if successful, false otherwise.
     */
    bool (*read_reg)(void *user_data, uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

    /** * @brief Blocks execution for a specified number of milliseconds.
     * @param ms Milliseconds to delay.
     */
    void (*delay_ms)(uint32_t ms);

    /** * @brief Retrieves the current system tick count in milliseconds.
     * @return Current system tick value.
     */
    uint32_t (*get_tick)(void);
} i2c_io_interface_t;

#endif /* __I2C_IO_H__ */