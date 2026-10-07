#ifndef TASK_NFC_READER_H
#define TASK_NFC_READER_H

#include "macro_utils.h"

/* ====================================================================
 * Application Protocol Configuration
 * ==================================================================== */
#define CMD_IMG_TRANSFER      0x01
#define CMD_FW_UPGRADE        0x02
#define CMD_ACK_MASK          0x80  /* Tag ACK response mask (e.g., 0x81 for Image ACK) */

#define STATUS_PASS           0x01
#define STATUS_FAIL           0x00

#define PROTOCOL_PACKET_SIZE  64
#define PROTOCOL_PAYLOAD_SIZE 59    /* 64 - 1(Cmd) - 2(Idx) - 2(CRC) = 59 Bytes */

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
    NFC_STATE_TRANS_SEND_CHUNK,      /* Pack and transmit 64-byte payload */
    NFC_STATE_TRANS_WAIT_ACK,        /* Poll NS_REG (0xED) for Tag MCU processing */
    NFC_STATE_TRANS_POLL_MCU_REPLY,  /* Read Tag's SRAM (0x3A) for ACK packet */
    NFC_STATE_TRANS_WAIT_EPD_UPDATE, /* Keep RF power ON until EPD finishes updating */
    NFC_STATE_TRANS_FAIL             /* Transmission failure handling */
} nfc_state_t;

/* Public Function Prototypes */
void Task_NFC_Reader_Init(void);
void Task_NFC_Reader(void);

/**
 * @brief Trigger the image transmission sequence. Call when button is pressed.
 */
void Task_NFC_TriggerImageTransfer(const unsigned char *image_data);
uint8_t Task_Image_Flag_Get(void);

#endif /* TASK_NFC_READER_H */