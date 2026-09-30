#include "MyApplication.h"

#define TASK_RS232_MAX_PAYLOAD_SIZE 128

/* application layer state management structure */
typedef struct {
    uint8_t  tx_buffer[TASK_RS232_MAX_PAYLOAD_SIZE]; /* internal buffer for tx payload */
    uint16_t tx_length;                              /* payload length */
    bool     tx_pending;                             /* flag indicating pending transmission */
} task_rs232_ctrl_t;

/* static instance of the control structure */
static task_rs232_ctrl_t s_rs232_ctrl;

/**
 * @brief initialize the rs232 application task layer.
 */
void task_rs232_init(void) {
    memset(&s_rs232_ctrl, 0, sizeof(task_rs232_ctrl_t));
    drv_rs232_init();
}

/**
 * @brief process rs232 rx parsing and tx scheduling.
 */
void task_rs232(void) {
    uint8_t rx_byte;

    /* --- rx processing --- */
    while (drv_rs232_available() > 0) {
        if (drv_rs232_read(&rx_byte)) {
            /* * TODO: implement protocol parsing logic here.
             * (e.g., buffering lines until '\n' or passing to a state machine)
             */
            printf("Rx: %02X\r\n", rx_byte);
        }
    }

    /* --- tx processing --- */
    if (s_rs232_ctrl.tx_pending) {
        /* pass the payload to the driver layer */
        drv_rs232_transmit(s_rs232_ctrl.tx_buffer, s_rs232_ctrl.tx_length);
        
        /* clear the pending flag so new transmissions can be accepted */
        s_rs232_ctrl.tx_pending = false;
    }
}

/**
 * @brief schedule data to be sent over the rs232 bus.
 */
int8_t task_rs232_send(const uint8_t *data, uint16_t length) {
    if (data == NULL || length == 0 || length > TASK_RS232_MAX_PAYLOAD_SIZE) {
        return -1; 
    }

    if (s_rs232_ctrl.tx_pending) {
        return -2; /* busy: previous payload is still pending */
    }

    /* copy payload into the internal buffer safely */
    memcpy(s_rs232_ctrl.tx_buffer, data, length);
    s_rs232_ctrl.tx_length = length;
    s_rs232_ctrl.tx_pending = true;

    return 0;
}

/**
 * @brief send a null-terminated string over the rs232 bus.
 */
int8_t task_rs232_send_string(const char *str) {
    if (str == NULL) {
        return -1;
    }
    
    uint16_t len = (uint16_t)strlen(str);
    return task_rs232_send((const uint8_t *)str, len);
}