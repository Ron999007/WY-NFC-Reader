/**
 * @file    task_sdcard.c
 * @brief   SD Card Task with Integrated Nuvoton M487 HAL
 * @details Fully independent async boot sequence. No delays, full speed.
 */

#include "MyApplication.h"       

/* ====================================================================
 * Nuvoton M487 Hardware Implementation
 * ==================================================================== */

#define SD_SECTOR_SIZE 512
static uint8_t bounce_buffer[SD_SECTOR_SIZE] __attribute__((aligned(4)));

void SDH0_IRQHandler(void) {
    uint32_t volatile isr_status = SDH0->INTSTS;
    if (SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk) {
        SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }
    if (isr_status & SDH_INTSTS_BLKDIF_Msk) {
        SD0.DataReadyFlag = TRUE;
    }
    SDH0->INTSTS = isr_status; 
}

static bool m480_is_inserted(void) { return (SDH_CardDetection(SDH0) != 0); }
static bool m480_init(void) {
    SYS_ResetModule(SDH0_RST);
    SDH_Open(SDH0, CardDetect_From_GPIO);
    SDH0->INTEN &= ~SDH_INTEN_CDIEN_Msk;
    NVIC_EnableIRQ(SDH0_IRQn);
    return (SDH_Probe(SDH0) == 0);
}
static void m480_deinit(void) { SDH0->GINTEN = 0; SDH0->INTEN = 0; }
static void m480_reset(void) {
    SYS_ResetModule(SDH0_RST);
    SDH_Open(SDH0, CardDetect_From_GPIO);
    SDH0->INTEN &= ~SDH_INTEN_CDIEN_Msk; 
    NVIC_EnableIRQ(SDH0_IRQn);
}

static bool m480_read_blocks(uint8_t *buff, uint32_t sector, uint32_t count) {
    if (((uint32_t)buff & 0x03) == 0) {
        return (SDH_Read(SDH0, buff, sector, count) == 0);
    } else {
        for (uint32_t i = 0; i < count; i++) {
            if (SDH_Read(SDH0, bounce_buffer, sector + i, 1) != 0) return false;
            memcpy(buff + (i * SD_SECTOR_SIZE), bounce_buffer, SD_SECTOR_SIZE);
        }
        return true;
    }
}

static bool m480_write_blocks(const uint8_t *buff, uint32_t sector, uint32_t count) {
    if (((uint32_t)buff & 0x03) == 0) {
        return (SDH_Write(SDH0, (uint8_t *)buff, sector, count) == 0);
    } else {
        for (uint32_t i = 0; i < count; i++) {
            memcpy(bounce_buffer, buff + (i * SD_SECTOR_SIZE), SD_SECTOR_SIZE);
            if (SDH_Write(SDH0, bounce_buffer, sector + i, 1) != 0) return false;
        }
        return true;
    }
}

static uint32_t m480_get_sector_count(void) { return SD0.totalSectorN; }

static const sd_hal_driver_t hal_sd_m480 = {
    .is_inserted      = m480_is_inserted,
    .init             = m480_init,
    .deinit           = m480_deinit,
    .reset            = m480_reset,
    .read_blocks      = m480_read_blocks,
    .write_blocks     = m480_write_blocks,
    .get_sector_count = m480_get_sector_count
};

/* ====================================================================
 * SD Card Task Logic & State Machine
 * ==================================================================== */

typedef enum {
    SD_STATE_WAIT_INSERT = 0,
    SD_STATE_PROBE,
    SD_STATE_MOUNT,
    SD_STATE_READ_INFO,
    SD_STATE_READY,
    SD_STATE_ERROR
} sd_task_state_t;

static sd_task_state_t current_sd_state = SD_STATE_WAIT_INSERT;
static uint32_t insert_timestamp = 0;
static uint32_t error_timestamp = 0;
static bool card_is_known_present = false;
static bool is_system_ready = false;

static FATFS fatfs_object;
static const TCHAR drive_path[] = { '0', ':', 0 };

void Task_SDCard_Init(void) {
    drv_sd_register_hal(&hal_sd_m480);
    current_sd_state = SD_STATE_WAIT_INSERT;
    insert_timestamp = 0;
    is_system_ready = false;
    card_is_known_present = false;
    printf("[SD Task] Initialization Complete. System Running at Full Speed.\n");
}

bool Task_SDCard_is_ready(void) { return is_system_ready; }

bool Task_SDCard_append_log(const char *filename, const char *message) {
    FIL file; FRESULT res; UINT bytes_written;
    if (!is_system_ready) return false; 

    res = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
    if (res == FR_OK) {
        res = f_write(&file, message, strlen(message), &bytes_written);
        f_close(&file);
        if (res == FR_OK && bytes_written == strlen(message)) return true;
    }
    return false;
}

void Task_SDCard(void) {
    bool is_inserted = drv_sd_is_card_inserted();

    if (!is_inserted) {
        if (card_is_known_present) {
            printf("\n[SD Task] Card physically removed! Unmounting...\n");
            card_is_known_present = false; 
            is_system_ready = false; 
            f_mount(NULL, drive_path, 0); 
            drv_sd_hardware_deinit();
            drv_sd_hardware_reset();
        }
        insert_timestamp = 0; 
        current_sd_state = SD_STATE_WAIT_INSERT;
        return; 
    }

    switch (current_sd_state) {
        case SD_STATE_WAIT_INSERT:
            if (is_inserted) {
                if (insert_timestamp == 0) {
                    insert_timestamp = Get_TickCount();
                } 
                /* 100ms Time-based Software Debounce (Physical contact stabilization) */
                else if ((Get_TickCount() - insert_timestamp) >= 100) {
                    printf("\n[SD Task] Card insertion verified. Probing hardware...\n");
                    card_is_known_present = true; 
                    insert_timestamp = 0; 
                    current_sd_state = SD_STATE_PROBE;
                }
            } else {
                insert_timestamp = 0;
            }
            break;

        case SD_STATE_PROBE:
            if (drv_sd_hardware_init()) {
                printf("[SD Task] Hardware Probe Success. Mounting FatFs...\n");
                current_sd_state = SD_STATE_MOUNT;
            } else {
                printf("[SD Task] Hardware Probe Failed. Will retry...\n");
                error_timestamp = Get_TickCount();
                current_sd_state = SD_STATE_ERROR;
            }
            break;

        case SD_STATE_MOUNT:
            if (f_mount(&fatfs_object, drive_path, 1) == FR_OK) {
                printf("[SD Task] FatFs Mount Success!\n");
                current_sd_state = SD_STATE_READ_INFO;
            } else {
                printf("[SD Task] FatFs Mount Failed. Will retry...\n");
                error_timestamp = Get_TickCount();
                current_sd_state = SD_STATE_ERROR;
            }
            break;

        case SD_STATE_READ_INFO:
            printf("\n========================================\n");
            printf("           SD CARD INFORMATION          \n");
            printf("========================================\n");
            printf(" - Capacity      : %u MB\n", (SD0.totalSectorN / 2048));
            printf(" - Total Sectors : %u\n", SD0.totalSectorN);
            printf(" - Sector Size   : %u Bytes\n", SD_SECTOR_SIZE);
            printf("========================================\n\n");
            printf("[SD Task] System is READY for File I/O.\n\n");
            
            is_system_ready = true;
            current_sd_state = SD_STATE_READY;
            break;

        case SD_STATE_READY:
            break;

        case SD_STATE_ERROR:
            if ((Get_TickCount() - error_timestamp) >= 1000) {
                printf("\n[SD Task] Retrying initialization sequence...\n");
                current_sd_state = SD_STATE_WAIT_INSERT; 
            }
            break;

        default:
            current_sd_state = SD_STATE_WAIT_INSERT;
            break;
    }
}