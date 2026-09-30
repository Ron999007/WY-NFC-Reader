/**
 * @file    task_eeprom.c
 * @brief   Implementation of the Bare-metal EEPROM Application Task.
 * @details Includes internal hardware I2C binding and ring buffer logic.
 */
#include "MyApplication.h"

/* ====================================================================
 * Hardware Adapter Layer (Internal Binding)
 * ==================================================================== */
static periph_i2c_obj_t eeprom_i2c_hw;

/** * @brief Wrapper for I2C Write to AT24C32D (Requires 16-bit internal address).
 */
static bool io_i2c_write(void *data, uint8_t addr, uint16_t reg, const uint8_t *buf, uint32_t len) {
    periph_i2c_obj_t *obj = (periph_i2c_obj_t *)data;
    if (obj == NULL || obj->i2c_base == NULL || buf == NULL || len == 0) return false;

    /* * Cast away 'const' from buf to match Nuvoton's non-const API signature.
     * This is safe because I2C_WriteMultiBytesTwoRegs does not modify the buffer.
     */
    uint32_t tx_len = I2C_WriteMultiBytesTwoRegs(obj->i2c_base, addr, reg, (uint8_t *)buf, len);
    return (tx_len == len);
}

/** * @brief Wrapper for I2C Read from AT24C32D (Requires 16-bit internal address).
 */
static bool io_i2c_read(void *data, uint8_t addr, uint16_t reg, uint8_t *buf, uint32_t len) {
    periph_i2c_obj_t *obj = (periph_i2c_obj_t *)data;
    if (obj == NULL || obj->i2c_base == NULL || buf == NULL || len == 0) return false;

    /* Use Nuvoton BSP API for 16-bit register addresses */
    uint32_t rx_len = I2C_ReadMultiBytesTwoRegs(obj->i2c_base, addr, reg, buf, len);
    return (rx_len == len);
}

static void io_delay(uint32_t ms) { 
    Delay_us(ms * 1000); 
}

static uint32_t io_get_tick(void) { 
    return Get_TickCount(); 
}

/* Internal IO Contract binding */
static i2c_io_interface_t eeprom_io_contract = {
    .user_data = &eeprom_i2c_hw, 
    .write_reg = (bool (*)(void*, uint8_t, uint8_t, const uint8_t*, uint32_t))io_i2c_write, /* Cast if interface uses uint8_t */
    .read_reg  = (bool (*)(void*, uint8_t, uint8_t, uint8_t*, uint32_t))io_i2c_read,
    .delay_ms  = io_delay, 
    .get_tick  = io_get_tick
};


/* ====================================================================
 * Task Core & Ring Buffer Variables
 * ==================================================================== */

typedef enum {
    EEPROM_CMD_READ = 0,
    EEPROM_CMD_WRITE
} eeprom_cmd_t;

typedef struct {
    eeprom_cmd_t      cmd;
    uint16_t          addr;
    uint8_t           *data;
    uint32_t          len;
    eeprom_callback_t callback;
    void              *user_data;
} eeprom_req_t;

#define TASK_EEPROM_MAX_REQUESTS 4
#define EEPROM_RB_STORAGE_SIZE ((TASK_EEPROM_MAX_REQUESTS * sizeof(eeprom_req_t)) + 1)

static drv_eeprom_handle_t g_eeprom_drv;
static Utils_RB_t          g_eeprom_rb;
static uint8_t             g_eeprom_rb_storage[EEPROM_RB_STORAGE_SIZE];

static eeprom_req_t        g_active_req;
static bool                g_is_processing = false;


/* ====================================================================
 * Private Functions
 * ==================================================================== */

static bool enqueue_request(eeprom_cmd_t cmd, uint16_t addr, uint8_t *data, uint32_t len, eeprom_callback_t cb, void *user_data) {
    eeprom_req_t req = {
        .cmd = cmd, .addr = addr, .data = data, .len = len,
        .callback = cb, .user_data = user_data
    };

    uint32_t used_bytes = Utils_RB_GetCount(&g_eeprom_rb);
    uint32_t free_bytes = g_eeprom_rb.capacity - 1 - used_bytes;

    if (free_bytes < sizeof(eeprom_req_t)) {
        return false; /* Queue is full */
    }

    uint8_t *ptr = (uint8_t *)&req;
    for (uint32_t i = 0; i < sizeof(eeprom_req_t); i++) {
        Utils_RB_Push(&g_eeprom_rb, &ptr[i]);
    }
    
    return true;
}


/* ====================================================================
 * Public API Implementation
 * ==================================================================== */

void task_eeprom_init(void) {
    /* Initialize hardware I2C port (Using I2C0 as an example to avoid conflict with OLED's I2C1) */
    periph_i2c_init(&eeprom_i2c_hw, I2C0, 200000);

    /* Bind the decoupled EEPROM hardware driver */
    drv_eeprom_init(&g_eeprom_drv, &eeprom_io_contract);
    
    /* Initialize the generic ring buffer utility */
    Utils_RB_Init(&g_eeprom_rb, g_eeprom_rb_storage, EEPROM_RB_STORAGE_SIZE, sizeof(uint8_t));
    
    g_is_processing = false;
}

void task_eeprom(void) {
    /* Step 1: Run the underlying driver state machine */
    if (g_is_processing) {
        drv_eeprom_run(&g_eeprom_drv);

        if (!drv_eeprom_is_busy(&g_eeprom_drv)) {
            bool success = (g_eeprom_drv.last_status == DRV_EEPROM_STATUS_OK);
            
            if (g_active_req.callback != NULL) {
                g_active_req.callback(success, g_active_req.user_data);
            }
            g_is_processing = false;
        }
    }

    /* Step 2: If idle, check the ring buffer for new complete requests */
    if (!g_is_processing && (Utils_RB_GetCount(&g_eeprom_rb) >= sizeof(eeprom_req_t))) {
        
        uint8_t *ptr = (uint8_t *)&g_active_req;
        for (uint32_t i = 0; i < sizeof(eeprom_req_t); i++) {
            Utils_RB_Pop(&g_eeprom_rb, &ptr[i]);
        }

        drv_eeprom_status_t status;
        
        if (g_active_req.cmd == EEPROM_CMD_WRITE) {
            status = drv_eeprom_write_req(&g_eeprom_drv, g_active_req.addr, g_active_req.data, g_active_req.len);
        } else {
            status = drv_eeprom_read_req(&g_eeprom_drv, g_active_req.addr, g_active_req.data, g_active_req.len);
        }

        if (status == DRV_EEPROM_STATUS_OK) {
            g_is_processing = true;
        } else {
            if (g_active_req.callback != NULL) {
                g_active_req.callback(false, g_active_req.user_data);
            }
        }
    }
}

bool task_eeprom_read(uint16_t addr, uint8_t *data, uint32_t len, eeprom_callback_t cb, void *user_data) {
    if (data == NULL || len == 0) return false;
    return enqueue_request(EEPROM_CMD_READ, addr, data, len, cb, user_data);
}

bool task_eeprom_write(uint16_t addr, const uint8_t *data, uint32_t len, eeprom_callback_t cb, void *user_data) {
    if (data == NULL || len == 0) return false;
    return enqueue_request(EEPROM_CMD_WRITE, addr, (uint8_t *)data, len, cb, user_data);
}

bool task_eeprom_is_queue_full(void) {
    uint32_t used_bytes = Utils_RB_GetCount(&g_eeprom_rb);
    uint32_t free_bytes = g_eeprom_rb.capacity - 1 - used_bytes;
    return (free_bytes < sizeof(eeprom_req_t));
}