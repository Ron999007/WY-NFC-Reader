#include "drv_rs232.h"
#include "periph_uart.h"
#include "utils_ringbuffer.h"
#include "NuMicro.h"
#include <stddef.h>

#define DRV_RS232_RX_BUF_SIZE 256
#define DRV_RS232_TX_BUF_SIZE 256

/* static memory allocation for ring buffers */
static uint8_t s_rx_storage[DRV_RS232_RX_BUF_SIZE];
static uint8_t s_tx_storage[DRV_RS232_TX_BUF_SIZE];
static Utils_RB_t s_rx_rb;
static Utils_RB_t s_tx_rb;

/* uart peripheral object instance */
static periph_uart_obj_t s_rs232_uart;

/**
 * @brief initialize the rs232 hardware, uart peripheral, and ring buffers.
 */
void drv_rs232_init(void) {
    /* 1. initialize software ring buffers */
    Utils_RB_Init(&s_rx_rb, s_rx_storage, DRV_RS232_RX_BUF_SIZE, sizeof(uint8_t));
    Utils_RB_Init(&s_tx_rb, s_tx_storage, DRV_RS232_TX_BUF_SIZE, sizeof(uint8_t));

    /* 2. initialize uart peripheral hardware 
     * bind to UART3 with 115200 baudrate 
     */
    periph_uart_init(&s_rs232_uart, UART3, 115200, &s_rx_rb, &s_tx_rb);
}

/**
 * @brief transmit data over the rs232 bus asynchronously.
 */
void drv_rs232_transmit(const uint8_t *data, uint32_t length) {
    if (data == NULL || length == 0) {
        return;
    }

    /* * push data to tx ring buffer and trigger uart thre interrupt.
     * unlike rs485, rs232 is full-duplex, so we do not need to block
     * and wait for the shift register to empty.
     */
    periph_uart_write(&s_rs232_uart, data, length);
}

/**
 * @brief get the number of available bytes in the rx ring buffer.
 */
uint32_t drv_rs232_available(void) {
    return periph_uart_available(&s_rs232_uart);
}

/**
 * @brief read a single byte from the rx ring buffer.
 */
bool drv_rs232_read(uint8_t *byte) {
    return periph_uart_read(&s_rs232_uart, byte);
}

/**
 * @brief m487 uart3 interrupt service routine.
 */
void UART3_IRQHandler(void) {
    periph_uart_irq_process(&s_rs232_uart);
}