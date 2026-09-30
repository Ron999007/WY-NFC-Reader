/**
 * @file    task_ymodem.c
 * @brief   YMODEM Task Implementation using UART0 (Shared with Console if necessary)
 */
#include "MyApplication.h"
#include "periph_uart.h" 

/* --- YMODEM Context --- */
static mid_ymodem_t ymodem_ctx;

/* --- UART Ring Buffers & Object --- */
#define UART_BUF_SIZE 2048
static Utils_RB_t rb_rx, rb_tx;
static uint8_t buf_rx[UART_BUF_SIZE], buf_tx[UART_BUF_SIZE];

/* Instantiate the UART object for YMODEM, now pointed to UART0 */
periph_uart_obj_t ymodem_uart;

/* ==================================================================== */
/* OTA Configuration Structure                                          */
/* ==================================================================== */
#define OTA_MAGIC_WORD 0x5AA5A55A

typedef struct {
    uint32_t magic_word;   /* 0x5AA5A55A to indicate valid data */
    uint32_t fw_size;      /* Size of the received file in bytes */
    uint8_t  padding[248]; /* Pad to 256 bytes for SPIM DMA alignment */
} OTA_Config_t;

/* ==================================================================== */
/* HAL Wrappers for YMODEM Middleware                                   */
/* ==================================================================== */

static bool HAL_UART_Read_Wrapper(uint8_t *data) { 
    return periph_uart_read(&ymodem_uart, data); 
}

static void HAL_UART_Write_Wrapper(uint8_t *data, uint32_t len) { 
    periph_uart_write(&ymodem_uart, data, len); 
}

static void HAL_UART_Flush_Wrapper(void) { 
    uint8_t dummy; 
    while (periph_uart_read(&ymodem_uart, &dummy)); 
}

/* Erase Wrapper: Trigger Async Erase and pump FSM until DONE */
static bool HAL_Flash_Erase_Wrapper(uint32_t addr) {
    if (!Task_Snor_Erase_Async(addr)) return false;
    
    while (1) {
        Task_Snor_Flash(); /* Pump the background flash state machine */
        Task_RGB_LED();
        snor_job_state_t state = Task_Snor_Get_State();
        
        if (state == SNOR_JOB_DONE) {
            Task_Snor_Clear_State();
            return true;
        } else if (state == SNOR_JOB_ERROR) {
            Task_Snor_Clear_State();
            return false;
        }
    }
}

/* Write Wrapper: Align buffer, trigger Async Write, and pump FSM until DONE */
static bool HAL_Flash_Write_Wrapper(uint32_t addr, const uint8_t *data, uint32_t len) {
    const uint8_t *target_data = data;
    
    if (((uint32_t)data % 4) != 0) {
        static uint32_t aligned_buf_32[256]; 
        uint8_t *aligned_buf = (uint8_t *)aligned_buf_32;
        memcpy(aligned_buf, data, len);
        target_data = aligned_buf;
    }
    
    if (!Task_Snor_Write_Async(addr, target_data, len)) return false;

    while (1) {
        Task_Snor_Flash(); 
        Task_RGB_LED();
        snor_job_state_t state = Task_Snor_Get_State();
        
        if (state == SNOR_JOB_DONE) {
            Task_Snor_Clear_State();
            return true;
        } else if (state == SNOR_JOB_ERROR) {
            Task_Snor_Clear_State();
            return false;
        }
    }
}

static bool HAL_Flash_Read_Wrapper(uint32_t addr, uint8_t *data, uint32_t len) {
    if (((uint32_t)data % 4) != 0) {
        static uint32_t aligned_buf_32[256]; 
        uint8_t *aligned_buf = (uint8_t *)aligned_buf_32;
        if (!Task_Snor_Read(addr, aligned_buf, len)) return false;
        memcpy(data, aligned_buf, len);
        return true;
    }
    return Task_Snor_Read(addr, data, len);
}

static uint32_t HAL_GetTick_Wrapper(void) {
    return Get_TickCount();
}

/* ==================================================================== */
/* Task State Machine                                                   */
/* ==================================================================== */
typedef enum {
    TASK_YM_IDLE = 0,
    TASK_YM_RX_RUNNING,
    TASK_YM_TX_RUNNING
} task_ymodem_state_e;

static task_ymodem_state_e current_task_state = TASK_YM_IDLE;

void Task_Ymodem_Init(void) {
    /* 1. Initialize Ring Buffers */
    Utils_RB_Init(&rb_rx, buf_rx, UART_BUF_SIZE, sizeof(uint8_t));
    Utils_RB_Init(&rb_tx, buf_tx, UART_BUF_SIZE, sizeof(uint8_t));
    
    /* 2. Initialize the UART object specifically for YMODEM using UART0 */
    periph_uart_init(&ymodem_uart, UART0, 115200, &rb_rx, &rb_tx);

    static mid_ymodem_hal_t ymodem_hal_cfg = {
        .uart_read          = HAL_UART_Read_Wrapper,
        .uart_write         = HAL_UART_Write_Wrapper,
        .uart_flush         = HAL_UART_Flush_Wrapper,
        .flash_erase_sector = HAL_Flash_Erase_Wrapper,
        .flash_write        = HAL_Flash_Write_Wrapper,
        .flash_read         = HAL_Flash_Read_Wrapper,
        .get_tick_ms        = HAL_GetTick_Wrapper
    };
    
    mid_ymodem_init(&ymodem_ctx, &ymodem_hal_cfg);
    current_task_state = TASK_YM_IDLE;
    
    printf("\r\n[TASK_YM] YMODEM Mode on UART0 Initialized.\r\n");
}

void Task_Ymodem(void) {
    uint8_t dummy_rx;
    
    switch (current_task_state) {
        case TASK_YM_IDLE:
            /* Poll data from UART0 */
            if (periph_uart_read(&ymodem_uart, &dummy_rx)) {
                if (dummy_rx == 'R' || dummy_rx == 'r') {
                    Task_RGB_StartBlink(0, 0, 100, 100);
                    mid_ymodem_clear_log();
                    HAL_UART_Flush_Wrapper();
                    mid_ymodem_receive_start(&ymodem_ctx, FIRMWARE_UPDATE_ADDR);
                    current_task_state = TASK_YM_RX_RUNNING;
                } 
                else if (dummy_rx == 'T' || dummy_rx == 't') {
                    Task_RGB_StartBlink(0, 100, 0, 100);
                    mid_ymodem_clear_log();
                    HAL_UART_Flush_Wrapper();
                    
                    OTA_Config_t cfg;
                    HAL_Flash_Read_Wrapper(OTA_CONFIG_ADDR, (uint8_t*)&cfg, sizeof(OTA_Config_t));
                    uint32_t tx_size = 10240; 
                    
                    if (cfg.magic_word == OTA_MAGIC_WORD && cfg.fw_size > 0) {
                        tx_size = cfg.fw_size;
                        printf("\r\n[TASK_YM] Dynamic File Size: %lu bytes.\r\n", (unsigned long)tx_size);
                    }

                    mid_ymodem_transmit_start(&ymodem_ctx, FIRMWARE_UPDATE_ADDR, "app_backup.bin", tx_size);
                    current_task_state = TASK_YM_TX_RUNNING;
                }
            }
            break;

        case TASK_YM_RX_RUNNING:
            {
                YMODEM_Status status = mid_ymodem_poll(&ymodem_ctx);
                if (status != YM_BUSY) {
                    if (status == YM_OK) {
                        Task_RGB_SetStaticColor(100, 0, 100);
                        printf("[TASK_YM] Receive SUCCESS!\r\n");
                        OTA_Config_t cfg = { .magic_word = OTA_MAGIC_WORD, .fw_size = ymodem_ctx.file_size };
                        HAL_Flash_Erase_Wrapper(OTA_CONFIG_ADDR);
                        HAL_Flash_Write_Wrapper(OTA_CONFIG_ADDR, (uint8_t*)&cfg, sizeof(OTA_Config_t));
                    } else {
                        Task_RGB_SetStaticColor(100, 0, 0);
                        printf("[TASK_YM] Receive FAILED!\r\n");
                    }
                    current_task_state = TASK_YM_IDLE;
                }
            }
            break;

        case TASK_YM_TX_RUNNING:
            {
                YMODEM_Status status = mid_ymodem_poll(&ymodem_ctx);
                if (status != YM_BUSY) {
                    Task_RGB_SetStaticColor(100, 0, 100);
                    printf("[TASK_YM] Transmit %s!\r\n", (status == YM_OK) ? "SUCCESS" : "FAILED");
                    current_task_state = TASK_YM_IDLE;
                }
            }
            break;

        default: 
            current_task_state = TASK_YM_IDLE; 
            break;
    }
}

void UART0_IRQHandler(void)
{
    periph_uart_irq_process(&ymodem_uart);
}