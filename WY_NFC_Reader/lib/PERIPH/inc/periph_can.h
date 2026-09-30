/**
 * @file    periph_can.h
 * @brief   Specific CAN Peripheral Driver Header.
 * @details Hardware peripheral driver definition for the CAN module.
 */
#ifndef __PERIPH_CAN_H__
#define __PERIPH_CAN_H__

#include "can_io.h"
#include "NuMicro.h" /* Nuvoton BSP */

/**
 * @brief CAN hardware object context structure.
 */
typedef struct {
    CAN_T       *can_base;    /**< Pointer to the CAN hardware base address (e.g., CAN0, CAN1) */
    uint32_t     irq_num;     /**< NVIC IRQ Number associated with the CAN module */
    can_rx_cb_t  app_cb;      /**< Application layer RX callback registered via the IO interface */
} periph_can_obj_t;

/**
 * @brief Initializes the specific CAN hardware peripheral.
 * @param obj Pointer to the CAN peripheral object instance.
 * @param can_base Pointer to the CAN register base (e.g., CAN0, CAN1).
 * @param baudrate Desired CAN bus baudrate in bps (e.g., 500000 for 500kbps).
 */
void periph_can_init(periph_can_obj_t *obj, CAN_T *can_base, uint32_t baudrate);

/**
 * @brief Binds the hardware object to the abstract CAN IO interface.
 * @param obj Pointer to the initialized CAN peripheral object.
 * @param out_interface Pointer to the generic interface structure to be populated.
 */
void periph_can_bind_interface(periph_can_obj_t *obj, can_io_interface_t *out_interface);

#endif /* __PERIPH_CAN_H__ */