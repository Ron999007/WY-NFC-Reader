/**
 * @file    periph_i2c.h
 * @brief   M487 Specific I2C Peripheral Driver Header.
 * @details Manages M487 hardware I2C ports and wraps BSP functions.
 */
#ifndef __PERIPH_I2C_H__
#define __PERIPH_I2C_H__

#include <stdint.h>
#include <stdbool.h>
#include "NuMicro.h" /* Nuvoton M487 BSP */

/**
 * @brief I2C Object capturing hardware state for a specific port.
 */
typedef struct {
    I2C_T *i2c_base; /* Pointer to the M487 hardware base (e.g., I2C0, I2C1) */
} periph_i2c_obj_t;

/**
 * @brief Initializes the I2C hardware port.
 * @param obj       Pointer to the I2C object instance.
 * @param i2c_base  M487 Hardware base address (e.g., I2C0).
 * @param bus_clock Target I2C bus clock frequency in Hz (e.g., 400000 for 400kHz).
 */
void periph_i2c_init(periph_i2c_obj_t *obj, I2C_T *i2c_base, uint32_t bus_clock);

/**
 * @brief Writes data bytes to a specific register on an I2C device.
 * @param obj        Pointer to the initialized I2C object.
 * @param slave_addr 7-bit I2C device address.
 * @param reg_addr   Internal register address of the device.
 * @param data       Pointer to the data payload.
 * @param len        Number of bytes to write.
 * @return true if successful, false otherwise.
 */
bool periph_i2c_write_reg(periph_i2c_obj_t *obj, uint8_t slave_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len);

/**
 * @brief Reads data bytes from a specific register on an I2C device.
 * @param obj        Pointer to the initialized I2C object.
 * @param slave_addr 7-bit I2C device address.
 * @param reg_addr   Internal register address of the device.
 * @param data       Pointer to the buffer to store received data.
 * @param len        Number of bytes to read.
 * @return true if successful, false otherwise.
 */
bool periph_i2c_read_reg(periph_i2c_obj_t *obj, uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len);

#endif /* __PERIPH_I2C_H__ */