#include "utils_ringbuffer.h"
#include <string.h> 

void Utils_RB_Init(Utils_RB_t *rb, void *storage, uint32_t capacity, uint32_t item_size) {
    if (rb == NULL || storage == NULL || capacity == 0 || item_size == 0) return;
    
    rb->buffer = (uint8_t *)storage;
    rb->capacity = capacity;
    rb->item_size = item_size;
    rb->head = 0;
    rb->tail = 0;
}

bool Utils_RB_IsEmpty(Utils_RB_t *rb) {
    if (rb == NULL) return true;
    return (rb->head == rb->tail);
}

bool Utils_RB_IsFull(Utils_RB_t *rb) {
    if (rb == NULL) return true;
    return (((rb->head + 1) % rb->capacity) == rb->tail);
}

uint32_t Utils_RB_GetCount(Utils_RB_t *rb) {
    if (rb == NULL) return 0;
    if (rb->head >= rb->tail) {
        return (rb->head - rb->tail);
    } else {
        return (rb->capacity - (rb->tail - rb->head));
    }
}

bool Utils_RB_Push(Utils_RB_t *rb, const void *data) {
    if (rb == NULL || data == NULL) return false;
    
    uint32_t next = (rb->head + 1) % rb->capacity;
    
    if (next == rb->tail) {
        return false; /* Buffer overflow */
    }
    
    /* Calculate the memory offset based on item size and copy data */
    uint32_t offset = rb->head * rb->item_size;
    memcpy(&(rb->buffer[offset]), data, rb->item_size);
    
    rb->head = next;
    return true;
}

bool Utils_RB_Pop(Utils_RB_t *rb, void *data) {
    if (rb == NULL || data == NULL) return false;
    
    if (Utils_RB_IsEmpty(rb)) {
        return false; /* Buffer is empty */
    }
    
    /* Calculate the memory offset based on item size and copy data out */
    uint32_t offset = rb->tail * rb->item_size;
    memcpy(data, &(rb->buffer[offset]), rb->item_size);
    
    rb->tail = (rb->tail + 1) % rb->capacity;
    return true;
}