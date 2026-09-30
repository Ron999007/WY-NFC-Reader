#include "MyApplication.h"

/* ====================================================================
 * Hardware Configuration (Adjust according to your passive buzzer)
 * ==================================================================== */
#define BUZZER_MUTE_DUTY    100     /* 0 for Active-High, 100 for Active-Low */
#define BUZZER_VOLUME_DUTY  50      /* 50 for max volume, 10 or 90 for lower volume */

/* ====================================================================
 * Debug Configuration
 * ==================================================================== */
#define BUZZER_DEBUG_ENABLE 1

#if BUZZER_DEBUG_ENABLE
    #define BUZ_LOG(fmt, ...) printf("[BUZ] " fmt "\r\n", ##__VA_ARGS__)
#else
    #define BUZ_LOG(fmt, ...)
#endif


/* ====================================================================
 * Mailbox Definitions (Inter-task Communication)
 * ==================================================================== */

typedef enum {
    BUZ_CMD_NONE = 0,
    BUZ_CMD_SET_MODE,
    BUZ_CMD_PLAY_MELODY
} buz_cmd_e;

typedef struct {
    buz_cmd_e cmd;
    union {
        struct { buzzer_mode_e mode; } simple;
        struct { const note_t *p_notes; uint16_t length; } melody;
    } params;
} buz_msg_t;

/* Mailbox variables for asynchronous command passing */
static volatile bool s_has_new_cmd = false;
static buz_msg_t s_mailbox;

/* ====================================================================
 * Private Hardware Context & Melody Definitions
 * ==================================================================== */

typedef struct {
    pwm_channel_e channel; 
} hw_pwm_ctx_t;

/* Static configuration for the buzzer's PWM channel */
static hw_pwm_ctx_t ctx_buzzer = { .channel = PWM_CHANNEL_5 };

/* Predefined Melody: Single Short Beep (2.7kHz for resonance) */
static const note_t beep_once[] = {
    {2700, 80}
};

/* Predefined Melody: Double Short Beep */
static const note_t beep_double[] = {
    {2700, 80}, {NOTE_REST, 60}, {2700, 80}
};

/* Predefined Melody: Fast Startup Song (Twinkle Twinkle) */
static const note_t song_twinkle[] = {
    {NOTE_C4, 150}, {NOTE_C4, 150}, {NOTE_G4, 150}, {NOTE_G4, 150},
    {NOTE_A4, 150}, {NOTE_A4, 150}, {NOTE_G4, 300}, {NOTE_REST, 100}
};

/* ====================================================================
 * Internal HAL Implementation
 * ==================================================================== */

/**
 * @brief Internal HAL: Initialize the specific PWM channel for the buzzer.
 */
static void hal_hw_buzzer_init(void) {
    pwm_config_t cfg = { 
        .frequency_hz = 1000, 
        .duty_cycle = BUZZER_MUTE_DUTY 
    };
    BUZ_LOG("HW Init - Channel: %u", (uint32_t)ctx_buzzer.channel);
    periph_pwm_init(ctx_buzzer.channel, &cfg);
    periph_pwm_enable(ctx_buzzer.channel);
}

/**
 * @brief Internal HAL: Set PWM frequency with volume and absolute mute logic.
 */
static void hal_hw_buzzer_set_freq(uint32_t freq) {
    if (freq == 0) {
        /* SILENCE: Output safe duty cycle and entirely disable the PWM output. 
         * This prevents any high-frequency glitches from imperfect 100% duty settings. 
         */
        periph_pwm_set_duty(ctx_buzzer.channel, BUZZER_MUTE_DUTY);
        periph_pwm_disable(ctx_buzzer.channel);
    } else {
        /* SOUND: Enable PWM, then output square wave at the target frequency. */
        periph_pwm_enable(ctx_buzzer.channel);
        periph_pwm_set_frequency(ctx_buzzer.channel, freq);
        periph_pwm_set_duty(ctx_buzzer.channel, BUZZER_VOLUME_DUTY);
    }
}

/* HAL interface structure for the driver layer injection */
static const buzzer_hardware_t HAL_HW_BUZZER = {
    .init = hal_hw_buzzer_init,
    .set_frequency = hal_hw_buzzer_set_freq
};

/* ====================================================================
 * Task Logic Implementation (State Machine)
 * ==================================================================== */

typedef enum {
    BUZ_STATE_IDLE = 0,
    BUZ_STATE_PLAYING,
    BUZ_STATE_GAP      /* Brief silence between notes to distinguish them */
} buz_state_e;

typedef struct {
    buz_state_e  state;
    const note_t *p_active_song;
    uint16_t     total_notes;
    uint16_t     current_idx;
    uint32_t     timer_ms;
} buz_ctrl_t;

static buz_ctrl_t g_buz_ctrl;

void Task_Buzzer_Init(void) {
    BUZ_LOG("Initializing Buzzer Task...");
    drv_buzzer_init(&HAL_HW_BUZZER);
    
    /* Reset state machine variables */
    g_buz_ctrl.state = BUZ_STATE_IDLE;
    g_buz_ctrl.p_active_song = NULL;
    g_buz_ctrl.total_notes = 0;
    
    s_has_new_cmd = false;
    BUZ_LOG("Task Initialization Complete.");
    
    Task_Buzzer_SetMode(BUZZER_MODE_BEEP_DOUBLE);
}

void Task_Buzzer(void) {
    static uint32_t u32_TickCnt_Temp = 0;
    uint32_t current_tick = Get_TickCount();

    /* 1. Process Mailbox (Asynchronous Command Processing) */
    if (s_has_new_cmd) {
        if (s_mailbox.cmd == BUZ_CMD_SET_MODE) {
            
            if (s_mailbox.params.simple.mode == BUZZER_MODE_BEEP_ONCE) {
                BUZ_LOG("Command: BEEP_ONCE");
                g_buz_ctrl.p_active_song = beep_once;
                g_buz_ctrl.total_notes = (uint16_t)(sizeof(beep_once)/sizeof(note_t));
                g_buz_ctrl.current_idx = 0;
                g_buz_ctrl.timer_ms = 0;
                g_buz_ctrl.state = BUZ_STATE_PLAYING;
            } 
            else if (s_mailbox.params.simple.mode == BUZZER_MODE_BEEP_DOUBLE) {
                BUZ_LOG("Command: BEEP_DOUBLE");
                g_buz_ctrl.p_active_song = beep_double;
                g_buz_ctrl.total_notes = (uint16_t)(sizeof(beep_double)/sizeof(note_t));
                g_buz_ctrl.current_idx = 0;
                g_buz_ctrl.timer_ms = 0;
                g_buz_ctrl.state = BUZ_STATE_PLAYING;
            }
            else if (s_mailbox.params.simple.mode == BUZZER_MODE_SONG_STARTUP) {
                BUZ_LOG("Command: PLAY_STARTUP_SONG");
                g_buz_ctrl.p_active_song = song_twinkle;
                g_buz_ctrl.total_notes = (uint16_t)(sizeof(song_twinkle)/sizeof(note_t));
                g_buz_ctrl.current_idx = 0;
                g_buz_ctrl.timer_ms = 0;
                g_buz_ctrl.state = BUZ_STATE_PLAYING;
            }

        } else if (s_mailbox.cmd == BUZ_CMD_PLAY_MELODY) {
            BUZ_LOG("Command: PLAY_CUSTOM_MELODY (%u notes)", (uint32_t)s_mailbox.params.melody.length);
            g_buz_ctrl.p_active_song = s_mailbox.params.melody.p_notes;
            g_buz_ctrl.total_notes = s_mailbox.params.melody.length;
            g_buz_ctrl.current_idx = 0;
            g_buz_ctrl.timer_ms = 0;
            g_buz_ctrl.state = BUZ_STATE_PLAYING;
        }
        s_has_new_cmd = false; /* Clear the mailbox flag */
    }

    /* 2. Execute Periodic Logic (1ms Resolution) */
    if (current_tick - u32_TickCnt_Temp >= 1) {
        u32_TickCnt_Temp = current_tick;

        if (g_buz_ctrl.state == BUZ_STATE_IDLE) {
            return;
        }

        g_buz_ctrl.timer_ms++;

        switch (g_buz_ctrl.state) {
            case BUZ_STATE_PLAYING:
                /* Trigger hardware update only on the first millisecond of the note */
                if (g_buz_ctrl.timer_ms == 1) {
                    uint32_t freq = (uint32_t)g_buz_ctrl.p_active_song[g_buz_ctrl.current_idx].freq;
                    
                    if (freq != 0) {
                        BUZ_LOG("Playing Note [%u/%u]: %u Hz", 
                                (uint32_t)(g_buz_ctrl.current_idx + 1), 
                                (uint32_t)g_buz_ctrl.total_notes, freq);
                    }
                    drv_buzzer_set_voice(freq);
                }
                
                /* Transition to silence gap once the duration expires */
                if (g_buz_ctrl.timer_ms >= g_buz_ctrl.p_active_song[g_buz_ctrl.current_idx].duration) {
                    g_buz_ctrl.timer_ms = 0;
                    g_buz_ctrl.state = BUZ_STATE_GAP;
                    drv_buzzer_set_voice(0); /* Silence output */
                }
                break;

            case BUZ_STATE_GAP:
                /* 15ms silence gap to make consecutive notes distinct */
                if (g_buz_ctrl.timer_ms >= 15) {
                    g_buz_ctrl.timer_ms = 0;
                    g_buz_ctrl.current_idx++;
                    
                    if (g_buz_ctrl.current_idx < g_buz_ctrl.total_notes) {
                        g_buz_ctrl.state = BUZ_STATE_PLAYING;
                    } else {
                        BUZ_LOG("Melody playback finished. Entering IDLE.");
                        g_buz_ctrl.state = BUZ_STATE_IDLE;
                    }
                }
                break;

            default:
                g_buz_ctrl.state = BUZ_STATE_IDLE;
                break;
        }
    }
}

/* ====================================================================
 * Public Control API Implementations (Mailbox Interface)
 * ==================================================================== */

void Task_Buzzer_SetMode(buzzer_mode_e mode) {
    s_mailbox.params.simple.mode = mode;
    s_mailbox.cmd = BUZ_CMD_SET_MODE;
    s_has_new_cmd = true;
}

void Task_Buzzer_PlayMelody(const note_t *p_melody, uint16_t length) {
    s_mailbox.params.melody.p_notes = p_melody;
    s_mailbox.params.melody.length = length;
    s_mailbox.cmd = BUZ_CMD_PLAY_MELODY;
    s_has_new_cmd = true;
}