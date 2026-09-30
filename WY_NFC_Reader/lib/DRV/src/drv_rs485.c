#include "drv_rs485.h"
#include "periph_uart.h"
#include "utils_ringbuffer.h"
#include "NuMicro.h"
#include <stddef.h>

#define RS485_DIR   PB9

#define DRV_RS485_RX_BUF_SIZE 256
#define DRV_RS485_TX_BUF_SIZE 256

/* static memory allocation for ring buffers */
static uint8_t s_rx_storage[DRV_RS485_RX_BUF_SIZE];
static uint8_t s_tx_storage[DRV_RS485_TX_BUF_SIZE];
static Utils_RB_t s_rx_rb;
static Utils_RB_t s_tx_rb;

/* uart peripheral object instance */
static periph_uart_obj_t s_rs485_uart;

/**
 * @brief Initialize the RS485 hardware, UART peripheral, and ring buffers.
 */
void drv_rs485_init(void) {
    /* 1. initialize software ring buffers */
    Utils_RB_Init(&s_rx_rb, s_rx_storage, DRV_RS485_RX_BUF_SIZE, sizeof(uint8_t));
    Utils_RB_Init(&s_tx_rb, s_tx_storage, DRV_RS485_TX_BUF_SIZE, sizeof(uint8_t));

    /* 2. configure PB9 as output for RS485 direction control */
    SYS->GPB_MFPH &= ~SYS_GPB_MFPH_PB9MFP_Msk;
    SYS->GPB_MFPH |= SYS_GPB_MFPH_PB9MFP_GPIO;
    GPIO_SetMode(PB, BIT9, GPIO_MODE_OUTPUT);
    RS485_DIR = 0; /* default to RX mode (receiver enable) */

    /* 3. initialize UART peripheral hardware (assuming 115200 baudrate) */
    periph_uart_init(&s_rs485_uart, UART0, 115200, &s_rx_rb, &s_tx_rb);
}

/**
 * @brief Transmit data over the RS485 bus safely in half-duplex mode.
 * * @param data Pointer to the data buffer.
 * @param length Number of bytes to transmit.
 */
void drv_rs485_transmit(const uint8_t *data, uint32_t length) {
    if (data == NULL || length == 0) {
        return;
    }

    /* pull PB9 high to enable RS485 transmitter */
    RS485_DIR = 1;

    /* push data to TX ring buffer and trigger UART THRE interrupt */
    periph_uart_write(&s_rs485_uart, data, length);

    /* * critical timing for RS485 half-duplex mode:
     * 1. wait until the interrupt completely empties the software TX ring buffer.
     */
    while (s_rs485_uart.tx_running) {
        /* blocking wait for TX interrupt to finish processing */
    }

    /* * 2. wait until the M487 hardware shift register completes sending the final bit.
     * if PB9 is pulled low before this, the last byte will be truncated on the bus.
     */
    while ((UART0->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk) == 0) {
        /* blocking wait for hardware shift register */
    }

    /* pull PB9 low to switch back to receiver mode */
    RS485_DIR = 0;
}

/**
 * @brief Get the number of available bytes in the RX ring buffer.
 * * @return Number of unread bytes.
 */
uint32_t drv_rs485_available(void) {
    return periph_uart_available(&s_rs485_uart);
}

/**
 * @brief Read a single byte from the RX ring buffer.
 * * @param byte Pointer to store the received byte.
 * @return true if a byte was successfully read, false otherwise.
 */
bool drv_rs485_read(uint8_t *byte) {
    return periph_uart_read(&s_rs485_uart, byte);
}

/**
 * @brief M487 UART0 Interrupt Service Routine.
 * Passes control to the peripheral driver to handle FIFO to Ring Buffer transfers.
 */
void UART0_IRQHandler(void) {
    periph_uart_irq_process(&s_rs485_uart);
}