#include "MyApplication.h"

#define TASK_RS485_MAX_PAYLOAD_SIZE 64

/* application layer state management structure */
typedef struct {
    uint8_t  tx_buffer[TASK_RS485_MAX_PAYLOAD_SIZE]; /* internal buffer for tx payload */
    uint16_t tx_length;                              /* payload length */
    bool     tx_pending;                             /* flag indicating pending transmission */
} task_rs485_ctrl_t;

/* static instance of the control structure */
static task_rs485_ctrl_t s_rs485_ctrl;

/**
 * @brief Initialize the RS485 application task layer.
 */
void task_rs485_init(void) {
    /* clear the control structure */
    memset(&s_rs485_ctrl, 0, sizeof(task_rs485_ctrl_t));
    
    /* initialize the underlying hardware driver */
    drv_rs485_init();
}

/**
 * @brief Process RS485 RX parsing and TX scheduling.
 * Must be called periodically in the main while(1) loop.
 */
void task_rs485(void) {
    uint8_t rx_byte;
    
    static uint32_t u32_TickCnt_Temp = 0;
       
    if(Get_TickCount() - u32_TickCnt_Temp >= 1000)
    {
        //task_rs485_send_string("RS485 System Ready!\r\n");
        u32_TickCnt_Temp += 1000;
    }

    /* * --- rx processing ---
     * continuously poll the software ring buffer. 
     * the UART IRQ fills it asynchronously in the background.
     */

    while (drv_rs485_available() > 0) {
        if (drv_rs485_read(&rx_byte)) {
            /* * TODO: implement protocol parsing logic here.
             * pass 'rx_byte' into your state machine (e.g., Modbus RTU parser).
             */
            printf("Rx: %02X\r\n", rx_byte);
        }
    }


    /* * --- tx processing ---
     * check if another task/module has scheduled data to send.
     */
    if (s_rs485_ctrl.tx_pending) {
        /* pass the payload to the driver layer for actual transmission */
        drv_rs485_transmit(s_rs485_ctrl.tx_buffer, s_rs485_ctrl.tx_length);
        
        /* clear the pending flag so new transmissions can be accepted */
        s_rs485_ctrl.tx_pending = false;
    }
}

/**
 * @brief Schedule data to be sent over the RS485 bus.
 * * @param data Pointer to the data buffer.
 * @param length Number of bytes to send.
 * @return 0 on success, negative error code on failure.
 */
int8_t task_rs485_send(const uint8_t *data, uint16_t length) {
    /* check for invalid parameters or buffer overflow */
    if (data == NULL || length == 0 || length > TASK_RS485_MAX_PAYLOAD_SIZE) {
        return -1; 
    }

    /* check if the transmitter is currently busy */
    if (s_rs485_ctrl.tx_pending) {
        return -2; /* busy: previous payload is still pending */
    }

    /* copy the payload into the internal buffer for safe deferred transmission */
    memcpy(s_rs485_ctrl.tx_buffer, data, length);
    s_rs485_ctrl.tx_length = length;
    
    /* set the flag to trigger transmission in the next process loop iteration */
    s_rs485_ctrl.tx_pending = true;

    return 0;
}

/**
 * @brief Send a null-terminated string over the RS485 bus.
 *
 * @param str Pointer to the null-terminated string.
 * @return 0 on success, negative error code on failure.
 */
int8_t task_rs485_send_string(const char *str) {
    if (str == NULL) {
        return -1;
    }
    
    /* Calculate the length of the string, excluding the null terminator */
    uint16_t len = (uint16_t)strlen(str);
    
    /* Reuse the core send function with appropriate type casting */
    return task_rs485_send((const uint8_t *)str, len);
}