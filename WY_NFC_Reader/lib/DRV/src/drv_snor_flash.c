#include "drv_snor_flash.h"

#define SNOR_TIMEOUT_MS  5000 /* 5 seconds maximum wait */

/* --- Private Helper Functions --- */

static bool snor_write_enable(drv_snor_t *dev) {
    snor_cmd_t cmd = {
        .instruction  = SNOR_CMD_WRITE_ENABLE,
        .inst_lines   = SNOR_LINE_1BIT,
        .addr_lines   = SNOR_LINE_NONE,
        .dummy_cycles = 0,
        .data_lines   = SNOR_LINE_NONE
    };
    return dev->hal->qspi_transmit(&cmd, NULL, 0);
}

static bool snor_wait_busy(drv_snor_t *dev) {
    uint8_t status = 0;
    uint32_t start_time = 0;
    
    if (dev->hal->get_tick_ms != NULL) {
        start_time = dev->hal->get_tick_ms();
    }

    snor_cmd_t cmd = {
        .instruction  = SNOR_CMD_READ_STATUS_1,
        .inst_lines   = SNOR_LINE_1BIT,
        .addr_lines   = SNOR_LINE_NONE,
        .dummy_cycles = 0,
        .data_lines   = SNOR_LINE_1BIT
    };

    do {
        if (!dev->hal->qspi_receive(&cmd, &status, 1)) return false;
        
        if (dev->hal->get_tick_ms != NULL) {
            if ((dev->hal->get_tick_ms() - start_time) > SNOR_TIMEOUT_MS) return false;
        }
    } while ((status & SNOR_SR1_WIP) == SNOR_SR1_WIP);

    return true;
}

static bool snor_page_program(drv_snor_t *dev, uint32_t addr, const uint8_t *buffer, uint16_t length) {
    if (!snor_write_enable(dev)) return false;

    /* Configured for standard 1-bit Page Program */
    snor_cmd_t cmd = {
        .instruction  = SNOR_CMD_PAGE_PROGRAM,
        .inst_lines   = SNOR_LINE_1BIT,
        .address      = addr,
        .addr_size    = 3, /* 24-bit address */
        .addr_lines   = SNOR_LINE_1BIT,
        .dummy_cycles = 0,
        .data_lines   = SNOR_LINE_1BIT
    };

    if (!dev->hal->qspi_transmit(&cmd, buffer, length)) return false;

    return snor_wait_busy(dev);
}

/* --- Public API Implementations --- */

bool drv_snor_init(drv_snor_t *dev, drv_snor_hal_t *hal) {
    if (dev == NULL || hal == NULL) return false;
    if (!hal->qspi_transmit || !hal->qspi_receive || !hal->delay_ms) return false;

    dev->hal = hal;
    dev->sector_size = 4096;
    dev->page_size = 256;
    dev->is_initialized = true;

    return true;
}

uint32_t drv_snor_read_id(drv_snor_t *dev) {
    if (!dev || !dev->is_initialized) return 0;

    uint8_t id_buf[3] = {0};
    uint32_t id = 0;

    snor_cmd_t cmd = {
        .instruction  = SNOR_CMD_READ_JEDEC_ID,
        .inst_lines   = SNOR_LINE_1BIT,
        .addr_lines   = SNOR_LINE_NONE,
        .dummy_cycles = 0,
        .data_lines   = SNOR_LINE_1BIT
    };

    if (dev->hal->qspi_receive(&cmd, id_buf, 3)) {
        id = (id_buf[0] << 16) | (id_buf[1] << 8) | id_buf[2];
    }
    return id;
}

void drv_snor_read(drv_snor_t *dev, uint32_t addr, uint8_t *buffer, uint32_t length) {
    if (!dev || !dev->is_initialized || length == 0) return;

    snor_wait_busy(dev);

    /* Configured for standard 1-bit Read Data */
    snor_cmd_t cmd = {
        .instruction  = SNOR_CMD_READ_DATA,
        .inst_lines   = SNOR_LINE_1BIT,
        .address      = addr,
        .addr_size    = 3,
        .addr_lines   = SNOR_LINE_1BIT,
        .dummy_cycles = 0,
        .data_lines   = SNOR_LINE_1BIT
    };

    dev->hal->qspi_receive(&cmd, buffer, length);
}

bool drv_snor_erase_sector(drv_snor_t *dev, uint32_t addr) {
    if (!dev || !dev->is_initialized) return false;
    if (!snor_write_enable(dev)) return false;

    snor_cmd_t cmd = {
        .instruction  = SNOR_CMD_SECTOR_ERASE_4K,
        .inst_lines   = SNOR_LINE_1BIT,
        .address      = addr,
        .addr_size    = 3,
        .addr_lines   = SNOR_LINE_1BIT,
        .dummy_cycles = 0,
        .data_lines   = SNOR_LINE_NONE
    };

    if (!dev->hal->qspi_transmit(&cmd, NULL, 0)) return false;
    return snor_wait_busy(dev);
}

bool drv_snor_write(drv_snor_t *dev, uint32_t addr, const uint8_t *buffer, uint32_t length) {
    if (!dev || !dev->is_initialized || length == 0) return false;

    uint16_t page_size = dev->page_size;
    uint16_t space_in_first_page = page_size - (addr % page_size);
    uint16_t chunk_size = (length <= space_in_first_page) ? length : space_in_first_page;

    /* Cross-page boundary program algorithm */
    while (length > 0) {
        if (!snor_page_program(dev, addr, buffer, chunk_size)) return false;

        addr += chunk_size;
        buffer += chunk_size;
        length -= chunk_size;
        chunk_size = (length <= page_size) ? length : page_size;
    }
    return true;
}