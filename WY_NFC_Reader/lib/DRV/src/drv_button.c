/**
 * @file    drv_button.c
 * @brief   Implementation of the highly portable button driver using an Event Queue.
 */

#include "drv_button.h"
#include <stddef.h>

/* --- Queue Configuration --- */
#define BTN_QUEUE_SIZE      16      /**< Maximum number of events the queue can hold */

/* --- Internal State Variables --- */
static button_t* s_btn_array = NULL;
static uint16_t         s_btn_count = 0;
static btn_hal_read_t   s_hal_read  = NULL;

/* --- Ring Buffer Variables --- */
static btn_msg_t        s_event_queue[BTN_QUEUE_SIZE];
static uint8_t          s_queue_head = 0;   /* Write index */
static uint8_t          s_queue_tail = 0;   /* Read index */

/**
 * @brief  [Internal] Push an event into the ring buffer.
 * @param  id    The ID of the button.
 * @param  event The event to be recorded.
 * @retval None
 */
static void Button_Push_Event(uint8_t id, btn_event_e event) 
{
    uint8_t next_head = (s_queue_head + 1) % BTN_QUEUE_SIZE;
    
    /* Push the event if the queue is not full */
    if (next_head != s_queue_tail) {
        s_event_queue[s_queue_head].id    = id;
        s_event_queue[s_queue_head].event = event;
        s_queue_head = next_head;
    }
    /* Note: If the queue is full, the oldest unread events are preserved, 
       and the newest event is dropped (overflow protection). */
}

/**
 * @brief  Initialize the button driver.
 * @param  btn_array     Pointer to the user-defined button array.
 * @param  count         Number of elements in the array.
 * @param  hal_read_func User-provided function to read hardware pin state.
 * @retval None
 */
void Button_Init(button_t* btn_array, uint16_t count, btn_hal_read_t hal_read_func) 
{
    s_btn_array = btn_array;
    s_btn_count = count;
    s_hal_read  = hal_read_func;
    
    /* Reset all internal dynamic states safely */
    if (s_btn_array != NULL) {
        for (uint16_t i = 0; i < s_btn_count; i++) {
            s_btn_array[i].state    = 0;
            s_btn_array[i].tick_cnt = 0;
        }
    }
    
    /* Reset Queue */
    s_queue_head = 0;
    s_queue_tail = 0;
}

/**
 * @brief  Read the oldest event from the button queue.
 * @note   This should be called continuously in the main application loop.
 * @param  msg Pointer to a user-provided structure to store the event.
 * @retval 1 if an event was read successfully, 0 if the queue is empty.
 */
uint8_t Button_Read_Event(btn_msg_t* msg) 
{
    /* Check if the queue is empty */
    if (s_queue_head == s_queue_tail) {
        return 0; 
    }
    
    /* Pop the oldest event */
    *msg = s_event_queue[s_queue_tail];
    s_queue_tail = (s_queue_tail + 1) % BTN_QUEUE_SIZE;
    
    return 1;
}

/**
 * @brief  Check if a specific button is currently logically pressed (debounced).
 * @param  id The unique ID of the button.
 * @retval 1 if logically pressed, 0 otherwise.
 */
uint8_t Button_Is_Pressed(uint8_t id) 
{
    if (s_btn_array == NULL) return 0;
    
    for (uint16_t i = 0; i < s_btn_count; i++) {
        if (s_btn_array[i].id == id) {
            return (s_btn_array[i].state > 0) ? 1 : 0;
        }
    }
    return 0;
}

/**
 * @brief  Returns a 32-bit mask representing the debounced state of all buttons.
 * @note   The bit position corresponds to the button's index in the array.
 * (e.g., Bit 0 represents s_btn_array[0])
 * @retval 32-bit unsigned integer mask.
 */
uint32_t Button_Get_State_Mask(void) 
{
    uint32_t mask = 0;
    
    if (s_btn_array == NULL) return 0;
    
    for (uint16_t i = 0; i < s_btn_count; i++) {
        /* If button state is 1 (Pressed) or 2 (Long Pressed), set the corresponding bit */
        if (s_btn_array[i].state > 0) {
            mask |= (1UL << i); 
        }
    }
    return mask;
}

/**
 * @brief  Core state machine to process button events.
 * @note   This function MUST be called periodically (e.g., every 10ms via Timer interrupt).
 * @retval None
 */
void Button_Ticks(void) 
{
    /* Safety check */
    if (s_btn_array == NULL || s_btn_count == 0 || s_hal_read == NULL) {
        return;
    }

    for (uint16_t i = 0; i < s_btn_count; i++) {
        button_t* target = &s_btn_array[i];
        
        /* 1. Read physical state via HAL callback */
        uint8_t physical_level = s_hal_read(target->port, target->pin);
        uint8_t is_pressed     = (physical_level == target->active_level) ? 1 : 0;
        
        /* 2. State Machine Logic */
        if (is_pressed) {
            target->tick_cnt++;
            
            /* Trigger Event: PRESS (Exactly at debounce threshold) */
            if (target->tick_cnt == BTN_DEBOUNCE_TICKS) {
                target->state = 1; /* Mark as debounced and pressed */
                Button_Push_Event(target->id, BTN_EVENT_PRESS);
            }
            
            /* Trigger Event: LONG PRESS (Exactly at long press threshold) */
            else if (target->tick_cnt == BTN_LONG_TICKS) {
                target->state = 2; /* Mark as long pressed */
                Button_Push_Event(target->id, BTN_EVENT_LONG_PRESS);
            }
            
        } else {
            /* If it was previously logically pressed */
            if (target->state > 0) {
                
                /* Trigger Event: SHORT CLICK (If released before long press) */
                if (target->state == 1) {
                    Button_Push_Event(target->id, BTN_EVENT_SHORT_CLICK);
                }
                
                /* Trigger Event: RELEASE (Always triggered upon physical release) */
                Button_Push_Event(target->id, BTN_EVENT_RELEASE);
            }
            
            /* Reset states completely */
            target->state    = 0;
            target->tick_cnt = 0;
        }
    }
}