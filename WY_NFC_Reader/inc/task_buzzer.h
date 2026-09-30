#ifndef TASK_BUZZER_H
#define TASK_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

/* ====================================================================
 * Data Types & Constants
 * ==================================================================== */

/**
 * @brief Structure for a single musical note.
 */
typedef struct {
    uint16_t freq;      /* Frequency in Hz. 0 for rest. */
    uint16_t duration;  /* Duration in milliseconds. */
} note_t;

/**
 * @brief Predefined buzzer modes.
 */
typedef enum {
    BUZZER_MODE_IDLE = 0,
    BUZZER_MODE_BEEP_ONCE,
    BUZZER_MODE_BEEP_DOUBLE,
    BUZZER_MODE_SONG_STARTUP, /* Example Song */
} buzzer_mode_e;

/* Standard Note Frequencies (Hz) */
#define NOTE_REST   0
#define NOTE_C4     262
#define NOTE_D4     294
#define NOTE_E4     330
#define NOTE_F4     349
#define NOTE_G4     392
#define NOTE_A4     440
#define NOTE_B4     494
#define NOTE_C5     523

/* ====================================================================
 * System Task API
 * ==================================================================== */

/**
 * @brief Initialize the buzzer task and internal hardware mapping.
 */
void Task_Buzzer_Init(void);

/**
 * @brief Main buzzer task process. Should be called periodically (e.g., in super-loop).
 */
void Task_Buzzer(void);

/* ====================================================================
 * Public Control API (Mailbox Access)
 * ==================================================================== */

/**
 * @brief Trigger a specific buzzer mode or song.
 * @param mode The selected buzzer mode.
 */
void Task_Buzzer_SetMode(buzzer_mode_e mode);

/**
 * @brief Play a custom melody array.
 * @param p_melody Pointer to the note_t array.
 * @param length Number of notes in the array.
 */
void Task_Buzzer_PlayMelody(const note_t *p_melody, uint16_t length);

#endif /* TASK_BUZZER_H */