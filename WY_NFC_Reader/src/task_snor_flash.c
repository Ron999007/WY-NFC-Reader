#include "MyApplication.h"

/* ============================================================================
 * 1. M487 SPIM HAL Implementation (Integrated Smart Fallback)
 * ============================================================================
 */

static uint32_t map_line_mode(snor_line_mode_t lines) {
    if (lines == SNOR_LINE_4BIT) return 4;
    if (lines == SNOR_LINE_2BIT) return 2;
    return 1;
}

/* Transmit command and data using M487 SPIM (Smart Routing with Strict DMA Rules) */
static bool m487_spim_transmit(snor_cmd_t *cmd, const uint8_t *tx_buffer, uint32_t length) {
    /* Condition for DMA: Buffer 4-byte aligned AND Length multiple of 4 */
    if ((((uint32_t)tx_buffer % 4) == 0) && ((length % 4) == 0) && (length > 0)) {
        if (cmd->instruction == SNOR_CMD_PAGE_PROGRAM || cmd->instruction == SNOR_CMD_QUAD_PAGE_PROGRAM) {
            if (cmd->data_lines == SNOR_LINE_4BIT) SPIM_SetQuadEnable(1, 1);
            uint32_t dma_cmd = (cmd->instruction << SPIM_CTL0_CMDCODE_Pos);
            SPIM_DMA_Write(cmd->address, (cmd->addr_size == 4), length, (uint8_t*)tx_buffer, dma_cmd);
            return true;
        }
    }

    /* Fallback: Pure Register PIO Mode */
    SPIM_SET_SS_EN(1);
    
    /* 1. Instruction Phase (Always Output) */
    if (cmd->inst_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_OUTPUT_MODE();
    else if (cmd->inst_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_OUTPUT_MODE();
    else SPIM_ENABLE_SING_OUTPUT_MODE();
    
    SPIM_SET_OPMODE(SPIM_CTL0_OPMODE_IO);
    SPIM_SET_DATA_WIDTH(8);
    SPIM_SET_DATA_NUM(1);
    SPIM->TX[0] = cmd->instruction;
    SPIM_SET_GO(); SPIM_WAIT_FREE();

    /* 2. Address Phase (Always Output) */
    if (cmd->addr_size > 0) {
        if (cmd->addr_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_OUTPUT_MODE();
        else if (cmd->addr_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_OUTPUT_MODE();
        else SPIM_ENABLE_SING_OUTPUT_MODE();

        for (int i = (cmd->addr_size - 1); i >= 0; i--) {
            SPIM->TX[0] = (cmd->address >> (i * 8)) & 0xFF;
            SPIM_SET_GO(); SPIM_WAIT_FREE();
        }
    }

    /* 3. Data Phase (Transmit -> Output) */
    if (length > 0 && tx_buffer != NULL) {
        if (cmd->data_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_OUTPUT_MODE();
        else if (cmd->data_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_OUTPUT_MODE();
        else SPIM_ENABLE_SING_OUTPUT_MODE();

        for (uint32_t i = 0; i < length; i++) {
            SPIM->TX[0] = tx_buffer[i];
            SPIM_SET_GO(); SPIM_WAIT_FREE();
        }
    }
    
    SPIM_SET_SS_EN(0);
    return true;
}

/* Transmit command and receive data using M487 SPIM (Smart Routing with Strict DMA Rules) */
static bool m487_spim_receive(snor_cmd_t *cmd, uint8_t *rx_buffer, uint32_t length) {
    /* Condition for DMA: Buffer 4-byte aligned AND Length multiple of 4 */
    if ((((uint32_t)rx_buffer % 4) == 0) && ((length % 4) == 0) && (length > 0)) {
        if (cmd->instruction == SNOR_CMD_READ_DATA || cmd->instruction == SNOR_CMD_FAST_READ || 
            cmd->instruction == SNOR_CMD_FAST_READ_QUAD_IO || cmd->instruction == SNOR_CMD_FAST_READ_QUAD_OUTPUT) {
            
            if (cmd->data_lines == SNOR_LINE_4BIT) SPIM_SetQuadEnable(1, 1);
            SPIM_SET_DCNUM(cmd->dummy_cycles); 
            
            uint32_t dma_cmd = (cmd->instruction << SPIM_CTL0_CMDCODE_Pos);
            SPIM_DMA_Read(cmd->address, (cmd->addr_size == 4), length, rx_buffer, dma_cmd, 1);
            
            if (cmd->data_lines == SNOR_LINE_4BIT) SPIM_SetQuadEnable(0, 1); 
            return true;
        }
    }

    /* Fallback: PIO Mode Receive */
    SPIM_SET_SS_EN(1);
    
    /* 1. Instruction Phase (Always Output) */
    if (cmd->inst_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_OUTPUT_MODE();
    else if (cmd->inst_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_OUTPUT_MODE();
    else SPIM_ENABLE_SING_OUTPUT_MODE();
    
    SPIM_SET_OPMODE(SPIM_CTL0_OPMODE_IO);
    SPIM_SET_DATA_WIDTH(8);
    SPIM_SET_DATA_NUM(1);
    SPIM->TX[0] = cmd->instruction;
    SPIM_SET_GO(); SPIM_WAIT_FREE();

    /* 2. Address Phase (Always Output) */
    if (cmd->addr_size > 0) {
        if (cmd->addr_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_OUTPUT_MODE();
        else if (cmd->addr_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_OUTPUT_MODE();
        else SPIM_ENABLE_SING_OUTPUT_MODE();

        for (int i = (cmd->addr_size - 1); i >= 0; i--) {
            SPIM->TX[0] = (cmd->address >> (i * 8)) & 0xFF;
            SPIM_SET_GO(); SPIM_WAIT_FREE();
        }
    }

    /* 3. Dummy Cycles Phase (Output Dummy Clocks) */
    if (cmd->dummy_cycles > 0) {
        if (cmd->data_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_OUTPUT_MODE();
        else if (cmd->data_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_OUTPUT_MODE();
        else SPIM_ENABLE_SING_OUTPUT_MODE();

        for (int i = 0; i < (cmd->dummy_cycles / 8); i++) {
            SPIM->TX[0] = 0x00;
            SPIM_SET_GO(); SPIM_WAIT_FREE();
        }
    }

    /* 4. Data Phase (Receive -> MUST SWITCH TO INPUT MODE) */
    if (length > 0 && rx_buffer != NULL) {
        
        /* [CRITICAL FIX]: Switch SPI pin direction to INPUT before reading! */
        if (cmd->data_lines == SNOR_LINE_4BIT) SPIM_ENABLE_QUAD_INPUT_MODE();
        else if (cmd->data_lines == SNOR_LINE_2BIT) SPIM_ENABLE_DUAL_INPUT_MODE();
        else SPIM_ENABLE_SING_INPUT_MODE();

        for (uint32_t i = 0; i < length; i++) {
            SPIM->TX[0] = 0xFF; /* Write dummy byte to generate SPI clock */
            SPIM_SET_GO(); SPIM_WAIT_FREE();
            rx_buffer[i] = (uint8_t)SPIM->RX[0]; /* Read actual data */
        }
    }
    
    SPIM_SET_SS_EN(0);
    return true;
}

drv_snor_hal_t m487_hal = {
    .qspi_transmit = m487_spim_transmit,
    .qspi_receive  = m487_spim_receive,
    .delay_ms      = CLK_SysTickDelay,
    .get_tick_ms   = NULL
};

drv_snor_t ext_flash;

/* ============================================================================
 * 2. Non-blocking State Machine Context
 * ============================================================================
 */
typedef struct {
    snor_job_state_t state;
    uint32_t         target_addr;
    const uint8_t    *write_buf;
    uint32_t         total_len;
    uint32_t         written_len;
} snor_fsm_t;

static snor_fsm_t snor_fsm = { .state = SNOR_JOB_IDLE };

static bool is_flash_busy(void) {
    uint8_t status = 0;
    snor_cmd_t cmd = { .instruction = SNOR_CMD_READ_STATUS_1, .inst_lines = SNOR_LINE_1BIT, .data_lines = SNOR_LINE_1BIT };
    m487_spim_receive(&cmd, &status, 1);
    return (status & SNOR_SR1_WIP) != 0;
}

static void send_wren(void) {
    snor_cmd_t cmd = { .instruction = SNOR_CMD_WRITE_ENABLE, .inst_lines = SNOR_LINE_1BIT };
    m487_spim_transmit(&cmd, NULL, 0);
}

/* ============================================================================
 * 3. Public API Implementation
 * ============================================================================
 */

void Task_Snor_Flash_Init(void) {
    SYS_UnlockReg();
    CLK_EnableModuleClock(SPIM_MODULE);
    SPIM_SET_CLOCK_DIVIDER(1);
    SPIM_SET_RXCLKDLY_RDDLYSEL(0);
    SPIM_SET_RXCLKDLY_RDEDGE();
    SPIM_InitFlash(1);
    SYS_LockReg();

    drv_snor_init(&ext_flash, &m487_hal);
    snor_fsm.state = SNOR_JOB_IDLE;
}

snor_job_state_t Task_Snor_Get_State(void) { return snor_fsm.state; }

void Task_Snor_Clear_State(void) {
    if (snor_fsm.state == SNOR_JOB_DONE || snor_fsm.state == SNOR_JOB_ERROR)
        snor_fsm.state = SNOR_JOB_IDLE;
}

bool Task_Snor_Read(uint32_t addr, uint8_t *buffer, uint32_t length) {
    if (snor_fsm.state == SNOR_JOB_BUSY_ERASE || snor_fsm.state == SNOR_JOB_BUSY_WRITE) return false;
    drv_snor_read(&ext_flash, addr, buffer, length);
    return true;
}

bool Task_Snor_Erase_Async(uint32_t addr) {
    if (snor_fsm.state != SNOR_JOB_IDLE) return false;
    send_wren();
    snor_cmd_t cmd = { .instruction = SNOR_CMD_SECTOR_ERASE_4K, .inst_lines = SNOR_LINE_1BIT, .address = addr, .addr_size = 3, .addr_lines = SNOR_LINE_1BIT };
    m487_spim_transmit(&cmd, NULL, 0);
    snor_fsm.state = SNOR_JOB_BUSY_ERASE;
    return true;
}

bool Task_Snor_Write_Async(uint32_t addr, const uint8_t *buffer, uint32_t length) {
    if (snor_fsm.state != SNOR_JOB_IDLE) return false;
    snor_fsm.target_addr = addr;
    snor_fsm.write_buf = buffer;
    snor_fsm.total_len = length;
    snor_fsm.written_len = 0;
    snor_fsm.state = SNOR_JOB_BUSY_WRITE;
    return true;
}

void Task_Snor_Flash(void) {
    if (snor_fsm.state == SNOR_JOB_IDLE || snor_fsm.state == SNOR_JOB_DONE || snor_fsm.state == SNOR_JOB_ERROR) return;

    /* Background Polling: Check if Flash internal operation is still WIP */
    if (is_flash_busy()) return;

    if (snor_fsm.state == SNOR_JOB_BUSY_ERASE) {
        snor_fsm.state = SNOR_JOB_DONE;
    } 
    else if (snor_fsm.state == SNOR_JOB_BUSY_WRITE) {
        if (snor_fsm.written_len < snor_fsm.total_len) {
            uint16_t page_sz = 256;
            uint16_t space = page_sz - (snor_fsm.target_addr % page_sz);
            uint32_t remain = snor_fsm.total_len - snor_fsm.written_len;
            uint16_t chunk = (remain <= space) ? (uint16_t)remain : space;

            send_wren();
            snor_cmd_t cmd = { .instruction = SNOR_CMD_PAGE_PROGRAM, .inst_lines = SNOR_LINE_1BIT, .address = snor_fsm.target_addr, .addr_size = 3, .addr_lines = SNOR_LINE_1BIT, .data_lines = SNOR_LINE_1BIT };
            m487_spim_transmit(&cmd, &snor_fsm.write_buf[snor_fsm.written_len], chunk);

            snor_fsm.target_addr += chunk;
            snor_fsm.written_len += chunk;
            /* Stay in BUSY_WRITE state to poll WIP in next tick */
        } else {
            snor_fsm.state = SNOR_JOB_DONE;
        }
    }
}