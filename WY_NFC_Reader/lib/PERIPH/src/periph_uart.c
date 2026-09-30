/**
 * @file    periph_uart.c
 * @brief   M487 Specific UART Peripheral Driver Implementation.
 */
#include "periph_uart.h"

/**
 * @brief Initializes the UART hardware and binds software buffers to the object.
 */
void periph_uart_init(periph_uart_obj_t *obj, UART_T *uart_base, uint32_t baudrate, Utils_RB_t *rx_rb, Utils_RB_t *tx_rb) {
    /* 1. Map parameters to the object instance */
    obj->uart_base = uart_base;
    obj->rx_rb = rx_rb;
    obj->tx_rb = tx_rb;
    obj->tx_running = false;

    /* 2. Configure M487 Hardware via BSP */
    UART_Open(obj->uart_base, baudrate);
    
    /* Optimize RX: Trigger interrupt after 4 bytes to reduce CPU overhead */
    obj->uart_base->FIFO = (obj->uart_base->FIFO & ~UART_FIFO_RFITL_Msk) | UART_FIFO_RFITL_4BYTES;
    UART_SetTimeoutCnt(obj->uart_base, 40);
    
    /* Enable Receive Data Available and Receive Timeout interrupts */
    UART_EnableInt(obj->uart_base, (UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk));
    
    /* 3. Enable System-wide Interrupt (NVIC) based on the selected port */
    if (obj->uart_base == UART0)      NVIC_EnableIRQ(UART0_IRQn);
    else if (obj->uart_base == UART1) NVIC_EnableIRQ(UART1_IRQn);
    else if (obj->uart_base == UART2) NVIC_EnableIRQ(UART2_IRQn);
    else if (obj->uart_base == UART3) NVIC_EnableIRQ(UART3_IRQn);
    else if (obj->uart_base == UART4) NVIC_EnableIRQ(UART4_IRQn);
    else if (obj->uart_base == UART5) NVIC_EnableIRQ(UART5_IRQn);
    else if (obj->uart_base == UART6) NVIC_EnableIRQ(UART6_IRQn);
    else if (obj->uart_base == UART7) NVIC_EnableIRQ(UART7_IRQn);
}

/**
 * @brief Checks the number of unread bytes in the RX ring buffer.
 */
uint32_t periph_uart_available(periph_uart_obj_t *obj) {
    if (obj == NULL || obj->rx_rb == NULL) return 0;
    return Utils_RB_GetCount(obj->rx_rb);
}

/**
 * @brief Pops one byte from the RX ring buffer.
 */
bool periph_uart_read(periph_uart_obj_t *obj, uint8_t *byte) {
    if (obj == NULL || obj->rx_rb == NULL) return false;
    return Utils_RB_Pop(obj->rx_rb, byte);
}

/**
 * @brief Pushes data into the TX ring buffer and triggers the TX interrupt if idle.
 */
void periph_uart_write(periph_uart_obj_t *obj, const uint8_t *data, uint32_t len) {
    if (obj == NULL || obj->tx_rb == NULL) return;

    /* Push all bytes into the Ring Buffer */
    for (uint32_t i = 0; i < len; i++) {
        /* Blocking wait if TX buffer is full to prevent data loss */
        while (Utils_RB_IsFull(obj->tx_rb)); 
        Utils_RB_Push(obj->tx_rb, &data[i]);
    }

    /* Start the TX engine if it is currently idle */
    if (!obj->tx_running) {
        obj->tx_running = true;
        /* Enable Transmit Holding Register Empty interrupt */
        UART_EnableInt(obj->uart_base, UART_INTEN_THREIEN_Msk);
    }
}

/**
 * @brief Core UART Interrupt processing logic.
 */
void periph_uart_irq_process(periph_uart_obj_t *obj) {
    if (obj == NULL || obj->uart_base == NULL) return;

    uint32_t u32IntSts = obj->uart_base->INTSTS;

    /* --- Process RX: Data Received or Timeout reached --- */
    if ((u32IntSts & UART_INTSTS_RDAINT_Msk) || (u32IntSts & UART_INTSTS_RXTOINT_Msk)) {
        /* Flush hardware FIFO into software Ring Buffer */
        while (UART_GET_RX_EMPTY(obj->uart_base) == 0) {
            uint8_t inChar = (uint8_t)UART_READ(obj->uart_base);
            if (obj->rx_rb != NULL) {
                Utils_RB_Push(obj->rx_rb, &inChar);
            }
        }
    }

    /* --- Process TX: Hardware is ready for more data --- */
    if (u32IntSts & UART_INTSTS_THREINT_Msk) {
        /* Fill hardware FIFO from software Ring Buffer */
        while (!UART_IS_TX_FULL(obj->uart_base)) {
            uint8_t outChar;
            if ((obj->tx_rb != NULL) && Utils_RB_Pop(obj->tx_rb, &outChar)) {
                UART_WRITE(obj->uart_base, outChar);
            } else {
                /* Buffer empty: stop TX interrupt and mark idle */
                UART_DisableInt(obj->uart_base, UART_INTEN_THREIEN_Msk);
                obj->tx_running = false;
                break;
            }
        }
    }
}