/**
 * @file    mid_ymodem.h
 * @brief   Non-blocking YMODEM Protocol Middleware Header
 */
#ifndef _MID_YMODEM_H_
#define _MID_YMODEM_H_

#include <stdint.h>
#include <stdbool.h>

/* YMODEM Operation Status Codes */
typedef enum {
    YM_OK = 0,      /* Operation completed successfully */
    YM_BUSY,        /* Operation is currently in progress */
    YM_ERROR,       /* General error occurred */
    YM_TIMEOUT,     /* Operation timed out after maximum retries */
    YM_ABORT        /* Transfer was aborted by user or remote host */
} YMODEM_Status;

/* Internal State Machine States */
typedef enum {
    /* --- Receive specific states --- */
    YM_STATE_IDLE = 0,
    YM_STATE_INIT,
    YM_STATE_WAIT_HEADER,
    YM_STATE_WAIT_PAYLOAD,
    YM_STATE_PROCESS_BLOCK,
    YM_STATE_EOT,
    
    /* --- Transmit specific states --- */
    YM_STATE_TX_WAIT_C,         /* Wait for initial 'C' from receiver */
    YM_STATE_TX_WAIT_ACK_BLK0,  /* Wait for ACK after sending Block 0 */
    YM_STATE_TX_WAIT_C_DATA,    /* Wait for 'C' before sending Data Blocks */
    YM_STATE_TX_SEND_BLOCK,     /* Send Data Block 1..N */
    YM_STATE_TX_WAIT_ACK_DATA,  /* Wait for ACK after sending a Data Block */
    YM_STATE_TX_WAIT_EOT_NAK,   /* Sent EOT #1, waiting for NAK */
    YM_STATE_TX_WAIT_EOT_ACK,   /* Sent EOT #2, waiting for ACK */
    YM_STATE_TX_WAIT_C_END,     /* Wait for 'C' before sending Null block */
    YM_STATE_TX_WAIT_ACK_NULL   /* Sent Null block, waiting for final ACK */
} YMODEM_State_t;

/* Hardware Abstraction Layer (HAL) Configuration */
typedef struct {
    bool     (*uart_read)(uint8_t *data); 
    void     (*uart_write)(uint8_t *data, uint32_t length);
    void     (*uart_flush)(void);
    
    bool     (*flash_erase_sector)(uint32_t address);
    bool     (*flash_write)(uint32_t address, const uint8_t *data, uint32_t length);
    bool     (*flash_read)(uint32_t address, uint8_t *data, uint32_t length);
    
    uint32_t (*get_tick_ms)(void); 
} mid_ymodem_hal_t;

/* YMODEM Context Structure (State Machine Memory) */
typedef struct {
    mid_ymodem_hal_t *hal;
    bool is_initialized;
    
    YMODEM_State_t state;
    uint32_t flash_ptr;
    uint32_t file_size;
    uint32_t remaining_bytes;
    uint32_t last_tick;
    
    uint8_t  packet_buf[1029];  /* Max YMODEM packet size */
    uint16_t rx_idx;
    uint16_t expected_size;
    
    uint8_t  error_count;
    uint8_t  eot_count;
    uint8_t  seq_num;
    bool     is_first_block;
    char     filename[64];
} mid_ymodem_t;

/* Public Middleware APIs */
bool mid_ymodem_init(mid_ymodem_t *ctx, mid_ymodem_hal_t *hal);
void mid_ymodem_receive_start(mid_ymodem_t *ctx, uint32_t flash_addr);
void mid_ymodem_transmit_start(mid_ymodem_t *ctx, uint32_t flash_addr, const char *name, uint32_t size);
YMODEM_Status mid_ymodem_poll(mid_ymodem_t *ctx);

/* Memory-Buffered Debug Logging APIs */
const char* mid_ymodem_get_log(void);
void mid_ymodem_clear_log(void);

#endif /* MID_YMODEM_H */