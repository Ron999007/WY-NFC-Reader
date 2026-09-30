/**
 * @file    periph_i2c.c
 * @brief   M487 Specific I2C Peripheral Driver Implementation.
 */
#include "periph_i2c.h"

/**
 * @brief Initializes the I2C hardware port.
 */
void periph_i2c_init(periph_i2c_obj_t *obj, I2C_T *i2c_base, uint32_t bus_clock) {
    if (obj == NULL || i2c_base == NULL) return;
    
    obj->i2c_base = i2c_base;
    I2C_Open(obj->i2c_base, bus_clock);
}

/**
 * @brief Writes data bytes to a specific register on an I2C device.
 */
bool periph_i2c_write_reg(periph_i2c_obj_t *obj, uint8_t slave_addr, uint8_t reg_addr, const uint8_t *data, uint32_t len) {
    if (obj == NULL || obj->i2c_base == NULL || data == NULL || len == 0) return false;

    /* Nuvoton BSP provides optimized functions based on length */
    if (len == 1) {
        uint8_t err = I2C_WriteByteOneReg(obj->i2c_base, slave_addr, reg_addr, data[0]);
        return (err == 0);
    } else {
        /* Cast const pointer because Nuvoton API expects a non-const pointer */
        uint32_t tx_len = I2C_WriteMultiBytesOneReg(obj->i2c_base, slave_addr, reg_addr, (uint8_t *)data, len);
        return (tx_len == len);
    }
}

/**
 * @brief Reads data bytes from a specific register on an I2C device.
 */
bool periph_i2c_read_reg(periph_i2c_obj_t *obj, uint8_t slave_addr, uint8_t reg_addr, uint8_t *data, uint32_t len) {
    if (obj == NULL || obj->i2c_base == NULL || data == NULL || len == 0) return false;

    if (len == 1) {
        data[0] = I2C_ReadByteOneReg(obj->i2c_base, slave_addr, reg_addr);
        return true; 
    } else {
        uint32_t rx_len = I2C_ReadMultiBytesOneReg(obj->i2c_base, slave_addr, reg_addr, data, len);
        return (rx_len == len);
    }
}