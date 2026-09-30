/**
 * @file    task_console.c
 * @brief   System Console Task with Asynchronous Non-blocking Architecture
 */

#include "MyApplication.h"
#include "sys_console.h"
#include "mid_shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ====================================================================
 * Configuration Macro 
 * ==================================================================== */
#define USE_USB_CONSOLE     1

#if USE_USB_CONSOLE
    #include "task_usb_vcom.h"
#else
    #include "periph_uart.h" 
    #include "utils_ringbuffer.h"
    
    #define SYS_RX_BUF_SIZE 256
    #define SYS_TX_BUF_SIZE 256
    static uint8_t app_rx_mem[SYS_RX_BUF_SIZE];
    static uint8_t app_tx_mem[SYS_TX_BUF_SIZE];
    static Utils_RB_t app_rx_rb;
    static Utils_RB_t app_tx_rb;
    periph_uart_obj_t console_uart;

    static void console_uart_write_wrapper(uint8_t *data, uint32_t len) {
        periph_uart_write(&console_uart, data, len);
    }
#endif

/* ====================================================================
 * Private Variables (Shell & Async State Machines)
 * ==================================================================== */
static char cmd_line_buf[SHELL_MAX_CMD_LINE];
static uint8_t cmd_idx = 0;

/* --- Async SD Card Print State Variables --- */
static bool     s_bIsPrinting = false;    /*!< Flag indicating a background SD print is active */
static bool     s_bPrintBinary = false;   /*!< Print mode: Text or Hex Dump */
static FIL      s_stPrintFile;            /*!< File object for the active print job */
static uint32_t s_u32PrintOffset = 0;     /*!< Current byte offset for Hex Dump */

/* --- Async SNOR Flash Read State Variables --- */
static bool     s_bIsSnorReading = false; /*!< Flag indicating a background SNOR read is active */
static uint32_t s_u32SnorReadAddr = 0;    /*!< Current address for background SNOR read */
static uint32_t s_u32SnorReadEnd = 0;     /*!< Target end address for SNOR read */

/* ====================================================================
 * Shell Command Implementations 
 * ==================================================================== */

static int cmd_help_func(shell_cmd_args *args) {
    Console_Printf("\r\n--- Available Commands ---\r\n");
    Console_Printf("help        : Show this menu\r\n");
    Console_Printf("clean       : Clear the screen\r\n");
    Console_Printf("\r\n--- Wi-Fi Commands ---\r\n");
    Console_Printf("mqttpub     : Publish MQTT msg (mqttpub <topic> <payload>)\r\n");
    Console_Printf("wifi_conn   : Connect to new AP (wifi_conn <ssid> <pwd>)\r\n");
    Console_Printf("\r\n--- SD Card Commands ---\r\n");
    Console_Printf("sd_ls       : List directory (sd_ls [path])\r\n");
    Console_Printf("sd_cat      : Read file asynchronously (sd_cat [-b] <filename>)\r\n");
    Console_Printf("\r\n--- SNOR Flash Commands ---\r\n");
    Console_Printf("snor_id     : Read SNOR JEDEC ID\r\n");
    Console_Printf("snor_status : Check background job status\r\n");
    Console_Printf("snor_erase  : Erase sector (snor_erase <hex_addr>)\r\n");
    Console_Printf("snor_write  : Write string (snor_write <hex_addr> <str>)\r\n");
    Console_Printf("snor_read   : Hex dump memory async (snor_read <hex_addr> <dec_len>)\r\n");
    return SHELL_PROCESS_OK;
}

static int cmd_clean_func(shell_cmd_args *args) {
    Console_Printf("\x1B[2J\x1B[H"); /* ANSI Escape sequences to clear screen and home cursor */
    return SHELL_PROCESS_OK;
}

static int cmd_sd_ls_func(shell_cmd_args *args) {
    const char *path = (args->count >= 1) ? args->args[0].val : "0:/";
    if (!Task_SDCard_is_ready()) {
        Console_Printf("\r\n[Error] SD Card is not ready!\r\n");
        return SHELL_PROCESS_OK;
    }
    DIR dir; FILINFO fno;
    if (f_opendir(&dir, path) == FR_OK) {
        Console_Printf("\r\n--- Directory: %s ---\r\n", path);
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
            Console_Printf(" %s %-20s %10lu Bytes\r\n", (fno.fattrib & AM_DIR) ? "[DIR]" : "[FILE]", fno.fname, fno.fsize);
        }
        f_closedir(&dir);
    } else {
        Console_Printf("\r\n[Error] Failed to open directory.\r\n");
    }
    return SHELL_PROCESS_OK;
}

static int cmd_sd_cat_func(shell_cmd_args *args) {
    if (s_bIsPrinting || s_bIsSnorReading) {
        Console_Printf("\r\n[Error] A background job is already running!\r\n");
        return SHELL_PROCESS_OK;
    }

    const char *filename = NULL;
    bool binary_mode = false;

    if (args->count < 1) {
        Console_Printf("\r\nUsage: sd_cat [-b] <filename>\r\n");
        return SHELL_PROCESS_OK;
    }

    for (int i = 0; i < args->count; i++) {
        if (strcmp(args->args[i].val, "-b") == 0) binary_mode = true;
        else filename = args->args[i].val;
    }

    if (!filename || !Task_SDCard_is_ready()) {
        Console_Printf("\r\n[Error] Invalid parameters or SD not ready.\r\n");
        return SHELL_PROCESS_OK;
    }

    /* Open the file. If successful, transition to the PRINTING state. */
    if (f_open(&s_stPrintFile, filename, FA_READ) == FR_OK) {
        s_bIsPrinting = true;
        s_bPrintBinary = binary_mode;
        s_u32PrintOffset = 0;
        
        Console_Printf("\r\n--- Reading File: %s (%s) ---\r\n", filename, binary_mode ? "Bin" : "Text");
        Console_Printf("--- [Press 'q' or 'Ctrl+C' to Abort] ---\r\n\r\n");
    } else {
        Console_Printf("\r\n[Error] Failed to open file '%s'.\r\n", filename);
    }
    return SHELL_PROCESS_OK;
}

static int cmd_mqtt_pub_func(shell_cmd_args *args) {
    if (args->count < 2) return SHELL_PROCESS_OK;
    Task_Wifi_Publish_MQTT(args->args[0].val, args->args[1].val);
    return SHELL_PROCESS_OK;
}

static int cmd_wifi_conn_func(shell_cmd_args *args) {
    if (args->count < 2) return SHELL_PROCESS_OK;
    char ssid_buf[32] = {0};
    const char *pwd = args->args[args->count - 1].val;
    for (int i = 0; i < args->count - 1; i++) {
        if (strlen(ssid_buf) + strlen(args->args[i].val) + 1 < sizeof(ssid_buf)) {
            strcat(ssid_buf, args->args[i].val);
            if (i < args->count - 2) strcat(ssid_buf, " ");
        }
    }
    Task_Wifi_Reconnect(ssid_buf, pwd);
    return SHELL_PROCESS_OK;
}

static int cmd_snor_id_func(shell_cmd_args *args) {
    if (ext_flash.is_initialized) Console_Printf("\r\nSNOR JEDEC ID: 0x%06X", drv_snor_read_id(&ext_flash));
    return SHELL_PROCESS_OK;
}

static int cmd_snor_status_func(shell_cmd_args *args) {
    Console_Printf("\r\nFlash Status: %d", Task_Snor_Get_State());
    Task_Snor_Clear_State(); 
    return SHELL_PROCESS_OK;
}

static int cmd_snor_erase_func(shell_cmd_args *args) {
    if (args->count >= 1) Task_Snor_Erase_Async(strtol(args->args[0].val, NULL, 16));
    return SHELL_PROCESS_OK;
}

static int cmd_snor_write_func(shell_cmd_args *args) {
    if (args->count >= 2) {
        static uint8_t wbuf[256];
        uint32_t len = strlen(args->args[1].val);
        if(len > 256) len = 256;
        memcpy(wbuf, args->args[1].val, len);
        Task_Snor_Write_Async(strtol(args->args[0].val, NULL, 16), wbuf, len);
    }
    return SHELL_PROCESS_OK;
}

static int cmd_snor_read_func(shell_cmd_args *args) {
    /* Prevent starting a new job if one is already running */
    if (s_bIsPrinting || s_bIsSnorReading) {
        Console_Printf("\r\n[Error] A background job is already running!\r\n");
        return SHELL_PROCESS_OK;
    }

    if (args->count < 2) {
        Console_Printf("\r\nUsage: snor_read <hex_addr> <dec_len>\r\n");
        return SHELL_PROCESS_OK;
    }

    /* Prevent reading if Flash is currently executing a background erase/write */
    if (Task_Snor_Get_State() == SNOR_JOB_BUSY_ERASE || Task_Snor_Get_State() == SNOR_JOB_BUSY_WRITE) {
        Console_Printf("\r\n[Error] Flash is busy! Please wait for background job to finish.\r\n");
        return SHELL_PROCESS_OK;
    }

    uint32_t start_addr = strtol(args->args[0].val, NULL, 16);
    uint32_t total_len = strtol(args->args[1].val, NULL, 10);

    if (total_len == 0) return SHELL_PROCESS_OK;

    /* Initialize the state machine variables for background reading */
    s_u32SnorReadAddr = start_addr;
    s_u32SnorReadEnd = start_addr + total_len;
    s_bIsSnorReading = true;

    Console_Printf("\r\n--- Async Reading %lu bytes from 0x%06X ---\r\n", total_len, start_addr);
    Console_Printf("--- [Press 'q' or 'Ctrl+C' to Abort] ---\r\n\r\n");
    
    return SHELL_PROCESS_OK;
}

/* ====================================================================
 * Command Table Registration
 * ==================================================================== */
static const shell_cmd my_cmd_table[] = {
    {"help",      "Show help",       cmd_help_func},
    {"clean",     "Clear screen",    cmd_clean_func}
#if 0
    {"mqttpub",   "MQTT Pub",        cmd_mqtt_pub_func},
    {"wifi_conn", "Wi-Fi Conn",      cmd_wifi_conn_func},
    {"sd_ls",     "List SD",         cmd_sd_ls_func},
    {"sd_cat",    "Async Read",      cmd_sd_cat_func},
    {"snor_id",   "SNOR ID",         cmd_snor_id_func},
    {"snor_status","SNOR Stat",      cmd_snor_status_func},
    {"snor_erase","SNOR Erase",      cmd_snor_erase_func},
    {"snor_write","SNOR Write",      cmd_snor_write_func},
    {"snor_read", "SNOR Read",       cmd_snor_read_func}
#endif
};

/* The struct uses the auto-calculated size for maximum flexibility */
static shell_cmds my_system_cmds = {
    .count = sizeof(my_cmd_table) / sizeof(my_cmd_table[0]),
    .cmds  = my_cmd_table
};

/* ====================================================================
 * Core Parsing Logic
 * ==================================================================== */
static void Process_Console_Char(uint8_t rx_data)
{
    /* Intercept 'q' or Ctrl+C to abort any active background job */
    if (s_bIsPrinting || s_bIsSnorReading) {
        if (rx_data == 'q' || rx_data == 'Q' || rx_data == 0x03) {
            if (s_bIsPrinting) {
                f_close(&s_stPrintFile);
            }
            s_bIsPrinting = false;
            s_bIsSnorReading = false;
            Console_Printf("\r\n\r\n--- [Job Aborted by User] ---\r\n> ");
        }
        return; /* Ignore all other inputs while working */
    }

    /* Normal Shell Input Processing */
    if (rx_data == '\b' || rx_data == 0x7F) {
        /* Handle Backspace visually and logically */
        if (cmd_idx > 0) { 
            cmd_idx--; 
            Console_Printf("\b \b"); 
        }
    } else if (rx_data == '\r' || rx_data == '\n') {
        /* Execute command on Enter */
        cmd_line_buf[cmd_idx] = '\0'; 
        if (cmd_idx > 0) {
            shell_process_cmds(&my_system_cmds, cmd_line_buf);
            cmd_idx = 0; 
        }
        /* Only print the prompt if NO background job was just started */
        if (!s_bIsPrinting && !s_bIsSnorReading) {
            Console_Printf("\r\n> "); 
        }
    } else if (rx_data >= 0x20 && rx_data <= 0x7E) {
        /* Store printable characters */
        if (cmd_idx < (SHELL_MAX_CMD_LINE - 1)) {
            cmd_line_buf[cmd_idx++] = (char)rx_data;
            Console_Printf("%c", rx_data); 
        }
    }
}

/* ====================================================================
 * Task API Implementation
 * ==================================================================== */
void Task_Console_Init(void) {
#if USE_USB_CONSOLE
    Task_USB_VCOM_Init();
    if (HSUSBD_IS_ATTACHED()) HSUSBD_Start();
    Console_Init((void *)VCOM_Write); 
#else
    Utils_RB_Init(&app_rx_rb, app_rx_mem, SYS_RX_BUF_SIZE, sizeof(uint8_t));
    Utils_RB_Init(&app_tx_rb, app_tx_mem, SYS_TX_BUF_SIZE, sizeof(uint8_t));
    periph_uart_init(&console_uart, UART0, 115200, &app_rx_rb, &app_tx_rb);
    Console_Init((void *)console_uart_write_wrapper);
#endif
    /* Reset state machines */
    s_bIsPrinting = false;
    s_bIsSnorReading = false;
    
    Console_Printf("\r\n=================================\r\n");
    Console_Printf(" System Console Ready. Type 'help'\r\n");
    Console_Printf("=================================\r\n> ");
}

void Task_Console(void) {
    /* ----------------------------------------------------------------
     * 1. Process Input Characters (Non-blocking)
     * ---------------------------------------------------------------- */
#if USE_USB_CONSOLE
    uint8_t rx_buf[64]; 
    uint32_t rx_len = VCOM_Read(rx_buf, sizeof(rx_buf));
    for (uint32_t i = 0; i < rx_len; i++) {
        Process_Console_Char(rx_buf[i]);
    }
#else
    uint8_t rx_data;
    while (periph_uart_read(&console_uart, &rx_data)) {
        Process_Console_Char(rx_data);
    }
#endif

    /* ----------------------------------------------------------------
     * 2. Process Background SD Print Job (Chunked Read)
     * ---------------------------------------------------------------- */
    if (s_bIsPrinting) {
        uint8_t buffer[64];
        UINT bytesRead;
        UINT chunkSize = s_bPrintBinary ? 16 : (sizeof(buffer) - 1);

        FRESULT res = f_read(&s_stPrintFile, buffer, chunkSize, &bytesRead);
        
        if (res != FR_OK || bytesRead == 0) {
            f_close(&s_stPrintFile);
            s_bIsPrinting = false;
            Console_Printf("\r\n--- End of File ---\r\n> ");
        } else {
            if (s_bPrintBinary) {
                Console_Printf("%08X: ", s_u32PrintOffset);
                for (int i = 0; i < 16; i++) {
                    if (i < bytesRead) Console_Printf("%02X ", buffer[i]);
                    else Console_Printf("   ");
                }
                Console_Printf(" | ");
                for (int i = 0; i < bytesRead; i++) {
                    char c = buffer[i];
                    Console_Printf("%c", (c >= 32 && c <= 126) ? c : '.');
                }
                Console_Printf("\r\n");
                s_u32PrintOffset += bytesRead;
            } else {
                buffer[bytesRead] = '\0';
                Console_Printf("%s", buffer);
            }
            
            /* Throttling delay to yield to USB driver */
            Delay_ms(2);
        }
    }

    /* ----------------------------------------------------------------
     * 3. Process Background SNOR Flash Read Job (Chunked Read)
     * ---------------------------------------------------------------- */
    if (s_bIsSnorReading) {
        uint8_t chunk_buf[16];
        char line_buf[128];
        
        /* Calculate how many bytes to read in this cycle (Max 16) */
        uint32_t chunk_size = (s_u32SnorReadEnd - s_u32SnorReadAddr >= 16) ? 16 : (s_u32SnorReadEnd - s_u32SnorReadAddr);
        
        /* Synchronous read from Flash (Fast execution) */
        Task_Snor_Read(s_u32SnorReadAddr, chunk_buf, chunk_size);
        
        /* Format the line string locally to minimize API overhead */
        int offset = 0;
        offset += snprintf(line_buf + offset, sizeof(line_buf) - offset, "%06X: ", s_u32SnorReadAddr);
        
        for (uint32_t i = 0; i < 16; i++) {
            if (i < chunk_size) {
                offset += snprintf(line_buf + offset, sizeof(line_buf) - offset, "%02X ", chunk_buf[i]);
            } else {
                offset += snprintf(line_buf + offset, sizeof(line_buf) - offset, "   ");
            }
        }
        
        offset += snprintf(line_buf + offset, sizeof(line_buf) - offset, " | ");
        
        for (uint32_t i = 0; i < chunk_size; i++) {
            char c = chunk_buf[i];
            line_buf[offset++] = (c >= 32 && c <= 126) ? c : '.';
        }
        
        line_buf[offset++] = '\r';
        line_buf[offset++] = '\n';
        line_buf[offset] = '\0';

        /* Transmit the pre-formatted line */
        Console_PutString(line_buf); 

        s_u32SnorReadAddr += chunk_size;

        /* Check if the job is complete */
        if (s_u32SnorReadAddr >= s_u32SnorReadEnd) {
            s_bIsSnorReading = false;
            Console_Printf("------------------------------------------------------------------\r\n> ");
        } else {
            /* Yield execution time to prevent USB VCOM TX buffer overflow */
            Delay_ms(2);
        }
    }
}

#if !USE_USB_CONSOLE
/* Route the UART hardware interrupt to the peripheral logic */
void UART0_IRQHandler(void) { periph_uart_irq_process(&console_uart); }
#endif