#ifndef DRV_SNOR_FLASH_H_
#define DRV_SNOR_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* --- Standard JEDEC Commands --- */
#define SNOR_CMD_WRITE_ENABLE          0x06
#define SNOR_CMD_READ_STATUS_1         0x05
#define SNOR_CMD_READ_DATA             0x03
#define SNOR_CMD_FAST_READ             0x0B
#define SNOR_CMD_FAST_READ_QUAD_OUTPUT 0x6B /* 1-bit CMD, 1-bit ADDR, 4-bit DATA */
#define SNOR_CMD_FAST_READ_QUAD_IO     0xEB /* 1-bit CMD, 4-bit ADDR, 4-bit DATA */
#define SNOR_CMD_PAGE_PROGRAM          0x02
#define SNOR_CMD_QUAD_PAGE_PROGRAM     0x32 /* 1-bit CMD, 1-bit ADDR, 4-bit DATA */
#define SNOR_CMD_SECTOR_ERASE_4K       0x20
#define SNOR_CMD_BLOCK_ERASE_32K       0x52
#define SNOR_CMD_BLOCK_ERASE_64K       0xD8
#define SNOR_CMD_CHIP_ERASE            0xC7
#define SNOR_CMD_READ_JEDEC_ID         0x9F

/* --- Status Register 1 Bits --- */
#define SNOR_SR1_WIP                   0x01  /* Write In Progress */
#define SNOR_SR1_WEL                   0x02  /* Write Enable Latch */

/* --- QSPI Transaction Definitions --- */

/* Definition of SPI line modes */
typedef enum {
    SNOR_LINE_NONE = 0,
    SNOR_LINE_1BIT = 1,
    SNOR_LINE_2BIT = 2,
    SNOR_LINE_4BIT = 4
} snor_line_mode_t;

/* QSPI Transaction Command Structure */
typedef struct {
    uint8_t          instruction;   /* e.g., 0xEB for Quad Read */
    snor_line_mode_t inst_lines;    /* Lines used for instruction phase */
    
    uint32_t         address;       /* Target flash address */
    uint8_t          addr_size;     /* Address size in bytes (usually 3 for 24-bit) */
    snor_line_mode_t addr_lines;    /* Lines used for address phase */
    
    uint8_t          dummy_cycles;  /* Number of dummy clock cycles */
    
    snor_line_mode_t data_lines;    /* Lines used for data phase */
} snor_cmd_t;

/* --- Hardware Abstraction Layer (HAL) Interface --- */
typedef struct {
    /* Transmit command and data to flash (e.g., Write Enable, Page Program) */
    bool (*qspi_transmit)(snor_cmd_t *cmd, const uint8_t *tx_buffer, uint32_t length);
    
    /* Transmit command and receive data from flash (e.g., Read ID, Fast Read) */
    bool (*qspi_receive)(snor_cmd_t *cmd, uint8_t *rx_buffer, uint32_t length);
    
    void (*delay_ms)(uint32_t ms);
    uint32_t (*get_tick_ms)(void);
} drv_snor_hal_t;

/* --- Flash Device Object --- */
typedef struct {
    drv_snor_hal_t *hal;
    uint32_t flash_size;
    uint16_t sector_size; /* Usually 4096 bytes */
    uint16_t page_size;   /* Usually 256 bytes */
    bool is_initialized;
} drv_snor_t;

/* --- Public API Declarations --- */

bool drv_snor_init(drv_snor_t *dev, drv_snor_hal_t *hal);
uint32_t drv_snor_read_id(drv_snor_t *dev);
void drv_snor_read(drv_snor_t *dev, uint32_t addr, uint8_t *buffer, uint32_t length);
bool drv_snor_erase_sector(drv_snor_t *dev, uint32_t addr);
bool drv_snor_write(drv_snor_t *dev, uint32_t addr, const uint8_t *buffer, uint32_t length);

#endif /* DRV_SNOR_FLASH_H_ */