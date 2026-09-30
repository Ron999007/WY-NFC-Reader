#ifndef TASK_NFC_READER_H
#define TASK_NFC_READER_H

#include "macro_utils.h"

/* ====================================================================
 * Image Transfer & EPD Configuration
 * ==================================================================== */
#define IMAGE_TOTAL_SIZE      12480
#define CHUNK_SIZE            64
#define TOTAL_CHUNKS          (IMAGE_TOTAL_SIZE / CHUNK_SIZE) /* 195 chunks */

/* Define the required time (in ms) to power the Tag for EPD update */
#define EPD_UPDATE_TIMEOUT_MS 10000  /* Adjust this based on your EPD's refresh time */

/* ====================================================================
 * NFC Reader Task States
 * ==================================================================== */
typedef enum {
    /* --- Original states from task_nfc_reader.h --- */
    NFC_STATE_INIT = 0,              /* Power on and hardware reset */
    NFC_STATE_LPCD_CONFIG,           /* Configure Low Power Card Detection */
    NFC_STATE_LPCD_WAITING,          /* Waiting for LPCD cycle interval */
    NFC_STATE_TAG_DETECTED,          /* Field change detected by BC45B4522 */
    NFC_STATE_TAG_SCANNING,          /* Execute REQA/Anticollision/Select */
    NFC_STATE_TAG_REPORT,            /* Process and print UID data */
    NFC_STATE_TAG_LEAVE_WAIT,        /* Cooling time before next detection */

    /* --- Additional states for image transmission & PassThrough mode --- */
    NFC_STATE_TRANS_INIT,            /* Prepare for transmission (Turn off antenna) */
    NFC_STATE_TRANS_WAIT_ANT_OFF,    /* Wait 500ms after antenna is turned off */
    NFC_STATE_TRANS_WAIT_ANT_ON,     /* Wait 1000ms after antenna is turned on */
    NFC_STATE_TRANS_SEND_CHUNK,      /* Pack and transmit 64-byte chunk of image data */
    NFC_STATE_TRANS_WAIT_ACK,        /* Wait 90ms for Tag ACK response */
    NFC_STATE_TRANS_WAIT_EPD_UPDATE, /* Keep RF power ON until EPD finishes updating */
    NFC_STATE_TRANS_FAIL
} nfc_state_t;

/* Public Function Prototypes */
void Task_NFC_Reader_Init(void);
void Task_NFC_Reader(void);

/**
 * @brief Trigger the image transmission sequence. Call when button is pressed.
 */
void Task_NFC_TriggerImageTransfer(void);

#endif /* TASK_NFC_READER_H */