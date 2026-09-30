/**
 * @file    mid_ymodem.c
 * @brief   Non-blocking YMODEM Protocol Middleware Implementation with Memory Logging
 */
#include "mid_ymodem.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* YMODEM Standard Control Characters */
#define YM_SOH 0x01
#define YM_STX 0x02
#define YM_EOT 0x04
#define YM_ACK 0x06
#define YM_NAK 0x15
#define YM_CAN 0x18
#define YM_C   0x43

/* Protocol Timing and Limits */
#define YM_TIMEOUT_MS   3000
#define YM_MAX_ERRORS   5
#define YM_SECTOR_SIZE  4096

/* --- Internal Memory Log Buffer --- */
static char ym_debug_log[2048] = {0};

/* Macro to append debug logs to the memory buffer without blocking UART */
#define YM_LOG(...) do { \
    char _tmp[64]; \
    snprintf(_tmp, sizeof(_tmp), __VA_ARGS__); \
    if (strlen(ym_debug_log) + strlen(_tmp) < 2040) { \
        strcat(ym_debug_log, _tmp); \
    } \
} while(0)

const char* mid_ymodem_get_log(void) {
    return ym_debug_log;
}

void mid_ymodem_clear_log(void) {
    ym_debug_log[0] = '\0';
}

/**
 * @brief Calculate CRC16-CCITT for YMODEM protocol.
 */
static uint16_t YMODEM_CalcCRC(const uint8_t *data, uint16_t length) {
    uint16_t crc = 0;
    while (length--) {
        crc = crc ^ (uint16_t)*data++ << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
            else crc = crc << 1;
        }
    }
    return crc;
}

bool mid_ymodem_init(mid_ymodem_t *ctx, mid_ymodem_hal_t *hal) {
    if (!ctx || !hal || !hal->flash_read) return false;
    memset(ctx, 0, sizeof(mid_ymodem_t));
    ctx->hal = hal;
    ctx->is_initialized = true;
    return true;
}

void mid_ymodem_receive_start(mid_ymodem_t *ctx, uint32_t flash_addr) {
    ctx->flash_ptr = flash_addr;
    ctx->is_first_block = true;
    ctx->error_count = 0;
    ctx->eot_count = 0;
    ctx->state = YM_STATE_INIT;
    YM_LOG("[RX] Task Started. Waiting for PC...\r\n");
}

void mid_ymodem_transmit_start(mid_ymodem_t *ctx, uint32_t flash_addr, const char *name, uint32_t size) {
    ctx->flash_ptr = flash_addr;
    ctx->file_size = size;
    ctx->remaining_bytes = size;
    ctx->seq_num = 0;
    ctx->error_count = 0;
    strncpy(ctx->filename, name, 63);
    ctx->state = YM_STATE_TX_WAIT_C;
    ctx->last_tick = ctx->hal->get_tick_ms();
    YM_LOG("[TX] Task Started. Waiting for PC to send 'C'...\r\n");
}

YMODEM_Status mid_ymodem_poll(mid_ymodem_t *ctx) {
    uint8_t rx_byte, resp;

    switch (ctx->state) {
        /* ========================================================== */
        /* --- RECEIVE STATES (PC to MCU)                         --- */
        /* ========================================================== */
        case YM_STATE_INIT:
            if (ctx->hal->uart_flush) ctx->hal->uart_flush();
            resp = YM_C; 
            ctx->hal->uart_write(&resp, 1);
            ctx->last_tick = ctx->hal->get_tick_ms();
            ctx->state = YM_STATE_WAIT_HEADER;
            YM_LOG("[RX] Sent 'C'\r\n");
            break;

        case YM_STATE_WAIT_HEADER:
            if (ctx->hal->uart_read(&rx_byte)) {
                if (rx_byte == YM_SOH || rx_byte == YM_STX) {
                    ctx->expected_size = (rx_byte == YM_STX) ? 1024 : 128;
                    ctx->packet_buf[0] = rx_byte;
                    ctx->rx_idx = 1;
                    ctx->state = YM_STATE_WAIT_PAYLOAD;
                    YM_LOG("[RX] Got Hdr: %s\r\n", (rx_byte == YM_STX) ? "STX(1K)" : "SOH(128)");
                } else if (rx_byte == YM_EOT) {
                    YM_LOG("[RX] Got EOT\r\n");
                    ctx->state = YM_STATE_EOT;
                } else if (rx_byte == YM_CAN) {
                    YM_LOG("[RX] Got CAN. Aborting.\r\n");
                    ctx->state = YM_STATE_IDLE; 
                    return YM_ABORT;
                }
                ctx->last_tick = ctx->hal->get_tick_ms();
            } else if ((ctx->hal->get_tick_ms() - ctx->last_tick) > YM_TIMEOUT_MS) {
                resp = (ctx->is_first_block) ? YM_C : YM_NAK; 
                ctx->hal->uart_write(&resp, 1);
                ctx->last_tick = ctx->hal->get_tick_ms();
                YM_LOG("[RX] Hdr Timeout! Sent %s\r\n", (ctx->is_first_block) ? "'C'" : "NAK");
                if (++ctx->error_count > YM_MAX_ERRORS) {
                    YM_LOG("[RX] Max errors reached!\r\n");
                    ctx->state = YM_STATE_IDLE; 
                    return YM_TIMEOUT;
                }
            }
            break;

        case YM_STATE_WAIT_PAYLOAD:
            while (ctx->hal->uart_read(&rx_byte)) {
                ctx->packet_buf[ctx->rx_idx++] = rx_byte;
                
                /* Refresh timeout timer on EVERY received byte to prevent false timeouts */
                ctx->last_tick = ctx->hal->get_tick_ms(); 
                
                if (ctx->rx_idx == (ctx->expected_size + 5)) {
                    ctx->state = YM_STATE_PROCESS_BLOCK;
                    break;
                }
            }
            
            if (ctx->state == YM_STATE_WAIT_PAYLOAD && (ctx->hal->get_tick_ms() - ctx->last_tick) > YM_TIMEOUT_MS) {
                resp = YM_NAK; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_WAIT_HEADER;
                YM_LOG("[RX] Payload Timeout! Sent NAK\r\n");
            }
            break;

        case YM_STATE_PROCESS_BLOCK:
            /* 1. Sequence Number Validation */
            if (ctx->packet_buf[1] != (uint8_t)(~ctx->packet_buf[2])) {
                resp = YM_NAK; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_WAIT_HEADER; 
                YM_LOG("[RX] Seq Err: Got %d, ~Got %d\r\n", ctx->packet_buf[1], ctx->packet_buf[2]);
                break;
            }
            
            /* 2. CRC16 Check Validation */
            uint16_t crc_rx = (ctx->packet_buf[ctx->expected_size + 3] << 8) | ctx->packet_buf[ctx->expected_size + 4];
            if (YMODEM_CalcCRC(&ctx->packet_buf[3], ctx->expected_size) != crc_rx) {
                resp = YM_NAK; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_WAIT_HEADER; 
                YM_LOG("[RX] CRC Err! Seq: %d\r\n", ctx->packet_buf[1]);
                break;
            }
            
            ctx->error_count = 0;

            if (ctx->eot_count >= 2) {
                resp = YM_ACK; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_IDLE; 
                YM_LOG("[RX] Final Null Block received. Transfer Complete!\r\n");
                return YM_OK;
            }

            if (ctx->is_first_block) {
                if (ctx->packet_buf[3] == 0) { 
                    resp = YM_ACK; 
                    ctx->hal->uart_write(&resp, 1);
                    ctx->state = YM_STATE_IDLE; 
                    YM_LOG("[RX] Empty Block 0 received. Aborting.\r\n");
                    return YM_ABORT;
                }
                
                char *size_str = (char *)&ctx->packet_buf[3 + strlen((char *)&ctx->packet_buf[3]) + 1];
                ctx->file_size = atoi(size_str);
                YM_LOG("[RX] Blk 0 OK. Size: %lu\r\n", (unsigned long)ctx->file_size);

                /* --- CRITICAL OPTIMIZATION: Removed the massive blocking Erase loop here! --- */
                /* Erase operation is now distributed (Just-In-Time) in the data block phase */
                
                ctx->is_first_block = false;
                uint8_t ack_c[] = {YM_ACK, YM_C}; 
                ctx->hal->uart_write(ack_c, 2);
            } 
            else {
                /* --- Just-In-Time (JIT) Sector Erase --- */
                /* Check if the current flash pointer is perfectly aligned with a 4KB sector boundary (4096 bytes). */
                /* If yes, it means we are entering a new sector, so we erase it before writing. */
                if ((ctx->flash_ptr % YM_SECTOR_SIZE) == 0) {
                    ctx->hal->flash_erase_sector(ctx->flash_ptr);
                }

                ctx->hal->flash_write(ctx->flash_ptr, &ctx->packet_buf[3], ctx->expected_size);
                ctx->flash_ptr += ctx->expected_size;
                
                resp = YM_ACK; 
                ctx->hal->uart_write(&resp, 1);
                YM_LOG("[RX] Blk %d OK. ACK sent\r\n", ctx->packet_buf[1]);
            }
            
            ctx->last_tick = ctx->hal->get_tick_ms();
            ctx->state = YM_STATE_WAIT_HEADER;
            break;

        case YM_STATE_EOT:
            ctx->eot_count++;
            if (ctx->eot_count == 1) {
                resp = YM_NAK; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_WAIT_HEADER;
                YM_LOG("[RX] EOT 1: Sent NAK\r\n");
            } else {
                uint8_t ack_c[] = {YM_ACK, YM_C}; 
                ctx->hal->uart_write(ack_c, 2);
                ctx->state = YM_STATE_WAIT_HEADER;
                YM_LOG("[RX] EOT 2: Sent ACK & 'C'\r\n");
            }
            ctx->last_tick = ctx->hal->get_tick_ms();
            break;


        /* ========================================================== */
        /* --- TRANSMIT STATES (MCU to PC)                        --- */
        /* ========================================================== */
        case YM_STATE_TX_WAIT_C:
            if (ctx->hal->uart_read(&rx_byte) && rx_byte == YM_C) {
                /* Receiver is ready. Construct Block 0 (Filename and Size) safely */
                memset(ctx->packet_buf, 0, 128 + 5);
                ctx->packet_buf[0] = YM_SOH; 
                ctx->packet_buf[1] = 0; 
                ctx->packet_buf[2] = 0xFF;
                
                /* --- CRITICAL FIX: Safe memory string construction --- */
                /* Avoid using sprintf with '%c' and '0' as it breaks in many embedded C libraries */
                char *payload_ptr = (char *)&ctx->packet_buf[3];
                
                /* 1. Copy the filename into the buffer */
                strcpy(payload_ptr, ctx->filename);
                
                /* 2. Move the pointer past the filename AND its null terminator */
                payload_ptr += strlen(ctx->filename) + 1; 
                
                /* 3. Convert and write the file size as an ASCII string */
                sprintf(payload_ptr, "%lu", (unsigned long)ctx->file_size);
                
                /* Calculate CRC for the 128-byte payload */
                uint16_t crc = YMODEM_CalcCRC(&ctx->packet_buf[3], 128);
                ctx->packet_buf[131] = (crc >> 8) & 0xFF; 
                ctx->packet_buf[132] = crc & 0xFF;
                
                /* Transmit Block 0 */
                ctx->hal->uart_write(ctx->packet_buf, 133);
                ctx->state = YM_STATE_TX_WAIT_ACK_BLK0;
                YM_LOG("[TX] Got 'C', Sent Blk 0 safely\r\n");
            }
            break;

        case YM_STATE_TX_WAIT_ACK_BLK0:
            if (ctx->hal->uart_read(&rx_byte)) {
                if (rx_byte == YM_ACK) {
                    ctx->state = YM_STATE_TX_WAIT_C_DATA;
                    YM_LOG("[TX] Blk 0 ACKed\r\n");
                } else if (rx_byte == YM_NAK) {
                    ctx->hal->uart_write(ctx->packet_buf, 133); 
                    YM_LOG("[TX] Blk 0 NAKed, Resent\r\n");
                } else if (rx_byte == YM_CAN) {
                    ctx->state = YM_STATE_IDLE; 
                    YM_LOG("[TX] Got CAN. Aborting.\r\n");
                    return YM_ABORT;
                }
            }
            break;

        case YM_STATE_TX_WAIT_C_DATA:
            if (ctx->hal->uart_read(&rx_byte) && rx_byte == YM_C) {
                ctx->state = YM_STATE_TX_SEND_BLOCK;
                YM_LOG("[TX] Got 'C', Starting Data Phase\r\n");
            }
            break;

        case YM_STATE_TX_SEND_BLOCK:
            if (ctx->remaining_bytes == 0) {
                resp = YM_EOT; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_TX_WAIT_EOT_NAK;
                YM_LOG("[TX] File Sent. Sent EOT 1\r\n");
                break;
            }
            
            uint16_t chunk = (ctx->remaining_bytes >= 1024) ? 1024 : 128;
            ctx->packet_buf[0] = (chunk == 1024) ? YM_STX : YM_SOH;
            ctx->packet_buf[1] = ++ctx->seq_num; 
            ctx->packet_buf[2] = ~ctx->seq_num;
            
            memset(&ctx->packet_buf[3], 0x1A, chunk);
            uint32_t to_read = (ctx->remaining_bytes > chunk) ? chunk : ctx->remaining_bytes;
            ctx->hal->flash_read(ctx->flash_ptr, &ctx->packet_buf[3], to_read);
            
            uint16_t c_crc = YMODEM_CalcCRC(&ctx->packet_buf[3], chunk);
            ctx->packet_buf[chunk + 3] = (c_crc >> 8) & 0xFF; 
            ctx->packet_buf[chunk + 4] = c_crc & 0xFF;
            
            ctx->hal->uart_write(ctx->packet_buf, chunk + 5);
            ctx->state = YM_STATE_TX_WAIT_ACK_DATA;
            YM_LOG("[TX] Sent Blk %d (Len: %d)\r\n", ctx->seq_num, chunk);
            break;

        case YM_STATE_TX_WAIT_ACK_DATA:
            if (ctx->hal->uart_read(&rx_byte)) {
                if (rx_byte == YM_ACK) {
                    uint16_t chunk_sent = (ctx->packet_buf[0] == YM_STX) ? 1024 : 128;
                    ctx->flash_ptr += chunk_sent;
                    ctx->remaining_bytes = (ctx->remaining_bytes > chunk_sent) ? (ctx->remaining_bytes - chunk_sent) : 0;
                    ctx->state = YM_STATE_TX_SEND_BLOCK;
                } else if (rx_byte == YM_NAK) {
                    uint16_t len = (ctx->packet_buf[0] == YM_STX) ? 1024 : 128;
                    ctx->hal->uart_write(ctx->packet_buf, len + 5);
                    YM_LOG("[TX] Blk NAKed, Resent\r\n");
                } else if (rx_byte == YM_CAN) {
                    ctx->state = YM_STATE_IDLE; 
                    return YM_ABORT;
                }
            }
            break;

        case YM_STATE_TX_WAIT_EOT_NAK:
            if (ctx->hal->uart_read(&rx_byte) && rx_byte == YM_NAK) {
                resp = YM_EOT; 
                ctx->hal->uart_write(&resp, 1);
                ctx->state = YM_STATE_TX_WAIT_EOT_ACK;
                YM_LOG("[TX] EOT 1 NAKed, Sent EOT 2\r\n");
            }
            break;

        case YM_STATE_TX_WAIT_EOT_ACK:
            if (ctx->hal->uart_read(&rx_byte) && rx_byte == YM_ACK) {
                ctx->state = YM_STATE_TX_WAIT_C_END;
                YM_LOG("[TX] EOT 2 ACKed, Wait for 'C'\r\n");
            }
            break;

        case YM_STATE_TX_WAIT_C_END:
            if (ctx->hal->uart_read(&rx_byte) && rx_byte == YM_C) {
                memset(ctx->packet_buf, 0, 128 + 5);
                ctx->packet_buf[0] = YM_SOH; 
                ctx->packet_buf[1] = 0; 
                ctx->packet_buf[2] = 0xFF;
                
                uint16_t crc = YMODEM_CalcCRC(&ctx->packet_buf[3], 128);
                ctx->packet_buf[131] = (crc >> 8) & 0xFF; 
                ctx->packet_buf[132] = crc & 0xFF;
                
                ctx->hal->uart_write(ctx->packet_buf, 133);
                ctx->state = YM_STATE_TX_WAIT_ACK_NULL;
                YM_LOG("[TX] Got 'C', Sent Null Blk 0\r\n");
            }
            break;

        case YM_STATE_TX_WAIT_ACK_NULL:
            if (ctx->hal->uart_read(&rx_byte) && rx_byte == YM_ACK) {
                ctx->state = YM_STATE_IDLE;
                YM_LOG("[TX] Null Blk ACKed. Complete.\r\n");
                return YM_OK; 
            }
            break;

        default: 
            return YM_OK;
    }
    return YM_BUSY;
}