#ifndef _TASK_DHT11_H_
#define _TASK_DHT11_H_

#include <stdint.h>

/* Non-blocking Application States */
typedef enum {
    TASK_DHT11_IDLE = 0,
    TASK_DHT11_WAITING_START_PULSE, /* Waiting for 18ms delay */
    TASK_DHT11_WAITING_DATA         /* Waiting for EXTI to decode 40 bits */
} Task_DHT11_State_t;

/* Public API */
void Task_DHT11_Init(void);
void Task_DHT11(void); /* Must be called continuously in while(1) loop */

#endif /* _TASK_DHT11_H_ */