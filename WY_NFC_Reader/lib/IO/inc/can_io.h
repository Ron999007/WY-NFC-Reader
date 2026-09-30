/**
 * @file    can_io.h
 * @brief   Abstract CAN Communication Interface (The Contract).
 * @details Decouples CAN application layers from specific MCU hardware drivers.
 */
#ifndef __CAN_IO_H__
#define __CAN_IO_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Standard CAN message structure.
 */
typedef struct {
    uint32_t id;          /**< CAN Message ID */
    uint8_t  dlc;         /**< Data Length Code (0-8 bytes) */
    uint8_t  data[8];     /**< Payload data buffer */
    bool     is_extended; /**< true if using Extended ID, false for Standard ID */
} can_msg_t;

/**
 * @brief Callback function pointer type for receiving CAN messages.
 * @param msg Pointer to the received CAN message structure.
 */
typedef void (*can_rx_cb_t)(const can_msg_t *msg);

/**
 * @brief Generic CAN IO Interface structure.
 * @details Application layers use this structure to interact with the underlying CAN bus 
 * without needing to know the specific MCU hardware registers.
 */
typedef struct {
    /**
     * @brief Opaque pointer to the hardware context (e.g., periph_can_obj_t).
     */
    void *user_data;

    /**
     * @brief Transmits a CAN message to the bus.
     * @param user_data Pointer to the hardware context.
     * @param msg Pointer to the CAN message to be sent.
     * @return true if the transmission request was successful, false otherwise.
     */
    bool (*transmit)(void *user_data, const can_msg_t *msg);

    /**
     * @brief Registers a callback function to handle incoming CAN messages.
     * @param user_data Pointer to the hardware context.
     * @param cb The callback function to be executed upon message reception.
     */
    void (*set_rx_callback)(void *user_data, can_rx_cb_t cb);

} can_io_interface_t;

#endif /* __CAN_IO_H__ */