#ifndef __UTILS_RINGBUFFER_H__
#define __UTILS_RINGBUFFER_H__

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Generic Ring Buffer Structure.
 * The actual memory buffer is allocated externally to keep this utility flexible.
 * It now supports any data type (e.g., uint8_t for UART, structs for CAN) via item_size.
 */
typedef struct {
    uint8_t *buffer;         /**< Pointer to the external memory array */
    uint32_t capacity;       /**< Maximum number of items the buffer can hold (was size) */
    uint32_t item_size;      /**< Size of a single item in bytes */
    volatile uint32_t head;  /**< Write index (modified by producer) */
    volatile uint32_t tail;  /**< Read index (modified by consumer) */
} Utils_RB_t;

/* API Declarations */

/**
 * @brief Initializes the ring buffer with external memory.
 * @param rb Pointer to the ring buffer instance.
 * @param storage Pointer to the external memory array.
 * @param capacity Maximum number of items the buffer can hold.
 * @param item_size Size of a single item in bytes (e.g., sizeof(uint8_t) or sizeof(can_msg_t)).
 */
void Utils_RB_Init(Utils_RB_t *rb, void *storage, uint32_t capacity, uint32_t item_size);

/**
 * @brief Checks if the ring buffer is empty.
 * @param rb Pointer to the ring buffer instance.
 * @return true if empty, false otherwise.
 */
bool Utils_RB_IsEmpty(Utils_RB_t *rb);

/**
 * @brief Checks if the ring buffer is full.
 * @param rb Pointer to the ring buffer instance.
 * @return true if full, false otherwise.
 */
bool Utils_RB_IsFull(Utils_RB_t *rb);

/**
 * @brief Gets the number of items currently stored in the ring buffer.
 * @param rb Pointer to the ring buffer instance.
 * @return Number of stored items.
 */
uint32_t Utils_RB_GetCount(Utils_RB_t *rb);

/**
 * @brief Pushes an item into the ring buffer.
 * @param rb Pointer to the ring buffer instance.
 * @param data Pointer to the item to be stored.
 * @return true if successful, false if the buffer is full.
 */
bool Utils_RB_Push(Utils_RB_t *rb, const void *data);

/**
 * @brief Pops an item from the ring buffer.
 * @param rb Pointer to the ring buffer instance.
 * @param data Pointer to the buffer where the popped item will be copied.
 * @return true if successful, false if the buffer is empty.
 */
bool Utils_RB_Pop(Utils_RB_t *rb, void *data);

#endif /* __UTILS_RINGBUFFER_H__ */