/**
 * @file    task_oled.c
 * @brief   Application Task for OLED UI Management.
 * @details Implements Non-blocking FSM, Configurable Marquee, Typewriter, 
 * and a clean Monochrome UI Layout with separator line.
 * Includes a 50ms Render Rate Limiter and a 2ms I2C Breathing Gap to 
 * prevent blocking CAN and LED tasks.
 */

#include "MyApplication.h"
#include <string.h>

/* ====================================================================
 * Bitmap Assets (Generated for LSB at Top)
 * ==================================================================== */

/** @brief Signal Strength Icons (12x8 pixels, Levels 0 to 4). */
static const uint8_t icon_signal[5][12] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xC0, 0xC0, 0x00, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0xC0, 0xC0, 0x00, 0xF0, 0xF0, 0x00, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00},
    {0xC0, 0xC0, 0x00, 0xF0, 0xF0, 0x00, 0xFC, 0xFC, 0x00, 0xFF, 0xFF, 0x00}
};

/** @brief Battery Level Icons (16x8 pixels, Levels 0 to 3). 
 * @details Fixed array to ensure 3rd block has proper spacing from the right wall.
 */
static const uint8_t icon_battery[4][16] = {
    /* Level 0: Empty */
    {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x3C},
    /* Level 1: 1 Block */
    {0xFF, 0x81, 0xBD, 0xBD, 0xBD, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x3C},
    /* Level 2: 2 Blocks */
    {0xFF, 0x81, 0xBD, 0xBD, 0xBD, 0x81, 0xBD, 0xBD, 0xBD, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, 0x3C},
    /* Level 3: Full (Perfectly spaced) */
    {0xFF, 0x81, 0xBD, 0xBD, 0xBD, 0x81, 0xBD, 0xBD, 0xBD, 0x81, 0xBD, 0xBD, 0xBD, 0x81, 0xFF, 0x3C}
};

/* ====================================================================
 * UI Mode and State Storage
 * ==================================================================== */
typedef enum {
    UI_MODE_STATIC = 0,
    UI_MODE_MARQUEE,
    UI_MODE_TYPEWRITER
} ui_mode_e;

static ui_mode_e current_ui_mode = UI_MODE_STATIC;

static char dashboard_text[64] = "Booting..."; 
static uint8_t current_signal_level = 4;
static uint8_t current_battery_level = 3;
static volatile bool flag_needs_refresh = true;

/* Animation State Variables */
static int16_t  marquee_x_offset = 128;          
static uint32_t marquee_speed = 30;      
static uint32_t last_anim_tick = 0;      

static uint32_t typewriter_speed = 100;
static uint32_t typewriter_pause = 1500;
static uint16_t typewriter_idx = 0;
static bool     typewriter_is_paused = false;

/* Dynamic FSM */
typedef enum {
    OLED_FSM_IDLE = 0, 
    OLED_FSM_UPDATE_PAGE
} oled_fsm_e;
static oled_fsm_e current_fsm_state = OLED_FSM_IDLE;
static uint8_t    current_update_page = 0;

static periph_i2c_obj_t i2c0_hw;
static ssd1306_obj_t    oled_drv;

/* ====================================================================
 * Hardware Adapter Layer
 * ==================================================================== */
static bool io_i2c_write(void *data, uint8_t addr, uint8_t reg, const uint8_t *buf, uint32_t len) {
    return periph_i2c_write_reg((periph_i2c_obj_t*)data, addr, reg, buf, len);
}
static bool io_i2c_read(void *data, uint8_t addr, uint8_t reg, uint8_t *buf, uint32_t len) {
    return periph_i2c_read_reg((periph_i2c_obj_t*)data, addr, reg, buf, len);
}
static void io_delay(uint32_t ms) { Delay_us(ms * 1000); }
static uint32_t io_get_tick(void) { return Get_TickCount(); } 

static i2c_io_interface_t oled_io_contract = {
    .user_data = &i2c0_hw, .write_reg = io_i2c_write, .read_reg = io_i2c_read,
    .delay_ms = io_delay, .get_tick = io_get_tick
};

/* ====================================================================
 * Public API Implementation
 * ==================================================================== */

void Task_OLED_Set_Dashboard(const char *text) {
    if (!text) return;
    current_ui_mode = UI_MODE_STATIC;
    strncpy(dashboard_text, text, sizeof(dashboard_text)-1);
    dashboard_text[sizeof(dashboard_text)-1] = '\0'; 
    flag_needs_refresh = true;
}

void Task_OLED_Set_Marquee(const char *text, uint32_t speed_ms) {
    if (!text) return;
    current_ui_mode = UI_MODE_MARQUEE;
    strncpy(dashboard_text, text, sizeof(dashboard_text)-1);
    dashboard_text[sizeof(dashboard_text)-1] = '\0';
    marquee_speed = speed_ms; 
    marquee_x_offset = 128; 
    last_anim_tick = io_get_tick();
    flag_needs_refresh = true;
}

void Task_OLED_Set_Typewriter(const char *text, uint32_t speed_ms, uint32_t pause_ms) {
    if (!text) return;
    current_ui_mode = UI_MODE_TYPEWRITER;
    strncpy(dashboard_text, text, sizeof(dashboard_text)-1);
    dashboard_text[sizeof(dashboard_text)-1] = '\0';
    typewriter_speed = speed_ms;
    typewriter_pause = pause_ms;
    typewriter_idx = 0;
    typewriter_is_paused = false;
    last_anim_tick = io_get_tick();
    flag_needs_refresh = true;
}

void Task_OLED_Set_Signal_Level(uint8_t level) {
    if (level > 4) level = 4; 
    if (current_signal_level != level) {
        current_signal_level = level;
        flag_needs_refresh = true;
    }
}

void Task_OLED_Set_Battery_Level(uint8_t level) {
    if (level > 3) level = 3; 
    if (current_battery_level != level) {
        current_battery_level = level;
        flag_needs_refresh = true;
    }
}

/* ====================================================================
 * Task Core Implementation
 * ==================================================================== */

void Task_OLED_Init(void) {
    /* ?? Fast Mode: Boost I2C to 100kHz to minimize hardware blocking time */
    periph_i2c_init(&i2c0_hw, I2C1, 100000);
    
    /* Initialize dynamically for 128x64 resolution */
    drv_oled_ssd1306_init(&oled_drv, &oled_io_contract, 0x3C, OLED_RES_128X64);
    
    Task_OLED_Set_Signal_Level(4);
    Task_OLED_Set_Battery_Level(3);    
    //Task_OLED_Set_Typewriter("Booting...", 100, 1500);
    //Task_OLED_Set_Marquee("Wha Yu NFC_Reader product~", 20);
    Task_OLED_Set_Dashboard("WhaYu NFC_Reader");
}

void Task_OLED(void) {
    uint32_t current_tick = oled_io_contract.get_tick();

    switch (current_fsm_state) {
        case OLED_FSM_IDLE:
            {
                static uint32_t last_render_tick = 0;

                /* 1. Process Animation Timings (Calculated instantly in RAM) */
                if (current_ui_mode == UI_MODE_MARQUEE) {
                    if (current_tick - last_anim_tick >= marquee_speed) {
                        last_anim_tick = current_tick;
                        marquee_x_offset -= 2; 
                        int16_t text_width = strlen(dashboard_text) * 8; 
                        if (marquee_x_offset < -text_width) marquee_x_offset = 128; 
                        flag_needs_refresh = true;
                    }
                } 
                else if (current_ui_mode == UI_MODE_TYPEWRITER) {
                    if (typewriter_is_paused) {
                        if (current_tick - last_anim_tick >= typewriter_pause) {
                            typewriter_is_paused = false;
                            typewriter_idx = 0; 
                            last_anim_tick = current_tick;
                            flag_needs_refresh = true;
                        }
                    } else {
                        if (typewriter_idx < strlen(dashboard_text)) {
                            if (current_tick - last_anim_tick >= typewriter_speed) {
                                last_anim_tick = current_tick;
                                typewriter_idx++;
                                flag_needs_refresh = true;
                            }
                        } else {
                            typewriter_is_paused = true;
                            last_anim_tick = current_tick;
                        }
                    }
                }

                /* 2. Render Framebuffer (Rate Limited to 50ms max refresh rate) */
                if (flag_needs_refresh && (current_tick - last_render_tick >= 50)) {
                    last_render_tick = current_tick;

                    drv_oled_ssd1306_clear(&oled_drv);
                    
                    /* Draw Top Status Bar Icons (Y = 0) */
                    drv_oled_ssd1306_draw_bitmap(&oled_drv, 0, 0, icon_signal[current_signal_level], 12, 8);
                    drv_oled_ssd1306_draw_bitmap(&oled_drv, 112, 0, icon_battery[current_battery_level], 16, 8);

                    /* Draw Separator Line (Y = 10) to make it look clean */
                    for (uint8_t i = 0; i < 128; i++) {
                        drv_oled_ssd1306_draw_pixel(&oled_drv, i, 10, true);
                    }

                    /* Center text in the remaining space (Y = 11 to 63) */
                    int16_t text_y_pos = 32;

                    if (current_ui_mode == UI_MODE_STATIC) {
                        int16_t text_width = strlen(dashboard_text) * 8; /* 1.5x scale width */
                        int16_t center_x = (text_width < 128) ? ((128 - text_width) / 2) : 0;
                        drv_oled_ssd1306_print(&oled_drv, center_x, text_y_pos, dashboard_text, OLED_SCALE_1_5X);
                    } 
                    else if (current_ui_mode == UI_MODE_MARQUEE) {
                        drv_oled_ssd1306_print(&oled_drv, marquee_x_offset, text_y_pos, dashboard_text, OLED_SCALE_1_5X);
                    }
                    else if (current_ui_mode == UI_MODE_TYPEWRITER) {
                        char temp_buf[64] = {0};
                        strncpy(temp_buf, dashboard_text, typewriter_idx);
                        
                        /* Always calculate X based on FULL string so text doesn't jump */
                        int16_t text_width = strlen(dashboard_text) * 8;
                        int16_t center_x = (text_width < 128) ? ((128 - text_width) / 2) : 0;
                        drv_oled_ssd1306_print(&oled_drv, center_x, text_y_pos, temp_buf, OLED_SCALE_1_5X);
                    }

                    /* Trigger FSM to update all pages sequentially via I2C */
                    current_update_page = 0;
                    current_fsm_state = OLED_FSM_UPDATE_PAGE;
                }
            }
            break;

        case OLED_FSM_UPDATE_PAGE: 
            {
                static uint32_t last_page_tick = 0;

                /* ?? BREATHING GAP: Enforce a strict 2ms delay between sending pages.
                   This guarantees the CPU yields back to the main loop so CAN 
                   and LED tasks can process their high-priority queues. */
                if (current_tick - last_page_tick >= 2) {
                    last_page_tick = current_tick;
                    
                    drv_oled_ssd1306_update_page(&oled_drv, current_update_page);
                    current_update_page++;
                    
                    if (current_update_page >= oled_drv.pages) {
                        /* Transfer complete: Clear flag and return to IDLE */
                        flag_needs_refresh = false;
                        current_fsm_state = OLED_FSM_IDLE;
                    }
                }
            }
            break;
    }
}