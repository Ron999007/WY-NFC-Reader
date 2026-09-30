/**
 * @file    task_can.h
 * @brief   CAN Bus Task Manager Header (Facade Pattern).
 * @details Handles asynchronous CAN message queuing and encapsulated hardware initialization.
 */
#ifndef __TASK_CAN_H__
#define __TASK_CAN_H__

#include "can_io.h"
#include "periph_can.h" /* Included to access hardware specific types like CAN_T */

/* Define Queue Capacities (Number of messages) */
#define CAN_RX_QUEUE_CAPACITY  16
#define CAN_TX_QUEUE_CAPACITY  16

/**
 * @brief CAN Task State Machine states.
 */
typedef enum {
    CAN_STATE_INIT = 0,     /**< Initialization state */
    CAN_STATE_IDLE,         /**< Waiting for events (RX or TX) */
    CAN_STATE_PROCESS_RX,   /**< Processing received messages from queue */
    CAN_STATE_PROCESS_TX,   /**< Processing transmit requests from queue */
    CAN_STATE_ERROR         /**< Error handling and recovery state */
} can_task_state_t;

/**
 * @brief Initializes the CAN Task Manager, including hardware and interface binding.
 * @param can_base Pointer to the specific CAN register base (e.g., CAN0 or CAN1).
 * @param baudrate Desired CAN bus baudrate in bps (e.g., 500000).
 */
void task_can_init(CAN_T *can_base, uint32_t baudrate);

/**
 * @brief Main execution function for the CAN task. Must be called periodically in the main loop.
 */
void task_can(void);

/**
 * @brief Queues a message to be transmitted by the state machine asynchronously.
 * @param msg Pointer to the CAN message structure to send.
 * @return true if successfully queued, false if the transmit queue is full.
 */
bool task_can_send_msg(const can_msg_t *msg);

#endif /* __TASK_CAN_H__ */