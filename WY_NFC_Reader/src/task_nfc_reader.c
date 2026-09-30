#include "MyApplication.h"
#include "bc45_cli.h"
#include "rfid.h"
#include "mifare.h"

/* Internal Task Variables */
static nfc_state_t g_nfc_state = NFC_STATE_INIT;
static uint32_t g_nfc_timer = 0;
static uint8_t g_scan_resp = 0;

/* Added declarations to fix "undeclared identifier" errors */
static char g_task_buffer[64];      /* Local buffer for UID string */
static uint16_t g_task_bufLen;      /* Variable to store UID length */
 
/* Allocate memory directly in this file */
uint8_t tx_buffer[128]; 
uint8_t rx_buffer[16];  
uint8_t ReceivedData[4];
uint8_t gu8_image_flag = 0;

/* Internal state tracking for image chunking */
static uint16_t chunk_idx = 0;     /* Counter for sent chunks (0 to 194) */
static uint16_t data_offset = 0;   /* Offset to read from image array */

uint8_t z; 
uint8_t k;
unsigned short crc16_2(unsigned char *data, int len, uint16_t init)
{
	unsigned short crc = init;		
 
	for ( z = 0; z < len; z++) {
		crc ^= data[z];
		for ( k = 0; k < 8; k++) {
			if (crc & 0x0001) {        
				crc = (crc >> 1) ^ 0x8408;
			}
			else {
				crc  = crc >> 1;
			}
		}
	}
	return crc;
}


/* ====================================================================
 * Wiegand Conversion Utilities
 * ==================================================================== */

/**
 * @brief  Convert 4-byte UID to Wiegand 34 (10-digit decimal)
 * @param  uid: Pointer to 4-byte UID array
 * @param  reverse_byte_order: Set to 1 for Little-Endian (reverse), 0 for Big-Endian
 * @return 32-bit Wiegand 34 Number
 */
static uint32_t Calculate_Wiegand34(uint8_t *uid, uint8_t reverse_byte_order) {
    uint32_t wg34_val = 0;
    
    if (reverse_byte_order) {
        /* [Byte3] [Byte2] [Byte1] [Byte0] */
        wg34_val = ((uint32_t)uid[3] << 24) | 
                   ((uint32_t)uid[2] << 16) | 
                   ((uint32_t)uid[1] << 8)  | 
                   ((uint32_t)uid[0]);
    } else {
        /* [Byte0] [Byte1] [Byte2] [Byte3] */
        wg34_val = ((uint32_t)uid[0] << 24) | 
                   ((uint32_t)uid[1] << 16) | 
                   ((uint32_t)uid[2] << 8)  | 
                   ((uint32_t)uid[3]);
    }
    return wg34_val;
}

/**
 * @brief  Convert 4-byte UID to Wiegand 26 (FC and CN)
 * @param  uid: Pointer to 4-byte UID array
 * @param  reverse_byte_order: Set to 1 for Little-Endian (reverse), 0 for Big-Endian
 * @param  fc: Pointer to store Facility Code (1 byte)
 * @param  cn: Pointer to store Card Number (2 bytes)
 */
static void Calculate_Wiegand26(uint8_t *uid, uint8_t reverse_byte_order, uint8_t *fc, uint16_t *cn) {
    uint32_t wg34_val = Calculate_Wiegand34(uid, reverse_byte_order);
    
    /* WG26 takes the lower 24 bits (3 bytes) of the 32-bit value */
    uint32_t lower_24bits = wg34_val & 0x00FFFFFF;
    
    *fc = (uint8_t)((lower_24bits >> 16) & 0xFF);     /* Top byte of the 24 bits */
    *cn = (uint16_t)(lower_24bits & 0xFFFF);          /* Lower 2 bytes of the 24 bits */
}

/* ====================================================================
 * NFC Reader Task
 * ==================================================================== */

/**
 * @brief Initialize NFC hardware and task state
 */
void Task_NFC_Reader_Init(void) {
    /* Set PB6 as NFC Reset pin */
    SYS->GPB_MFPL &= ~SYS_GPB_MFPL_PB6MFP_Msk;
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB6MFP_GPIO;
    GPIO_SetMode(PB, (BIT6), GPIO_MODE_OUTPUT);
    NFC_RESET = 1;
    
    /* Set PB1 as NFC IRQ pin */
    SYS->GPB_MFPL &= ~SYS_GPB_MFPL_PB1MFP_Msk;
    SYS->GPB_MFPL |= SYS_GPB_MFPL_PB1MFP_GPIO;
    GPIO_SetMode(PB, (BIT1), GPIO_MODE_INPUT);
    
    pcd_poweron();          /* Hardware VCC/RST control */
    pcd_init();             /* Register initialization */
    pcd_antenna_reset();    /* Ensure antenna field is clear */
    BC45_Configuration(CONFIG_14443A);
    g_nfc_state = NFC_STATE_INIT;
    //g_nfc_state = NFC_STATE_TRANS_INIT;
}

/**
 * @brief Trigger the background image transmission sequence.
 *        Usually called from the external Button Task when pressed.
 */
void Task_NFC_TriggerImageTransfer(void) 
{
    /* Only allow triggering if it is in normal scanning states */
    if (g_nfc_state <= NFC_STATE_TAG_LEAVE_WAIT) { 
        gu8_image_flag = ~gu8_image_flag;
        g_nfc_state = NFC_STATE_TRANS_INIT;
        drv_led_set_blink(&g_leds[LED_ID_G], 100, 100);
    }
}

/**
 * @brief Non-blocking NFC Task State Machine
 * This is a decomposed version of the original blocking ScanUID logic.
 */
void Task_NFC_Reader(void) {
    
    /* Variables for transmission */
    uint16_t crc_val;
    uint8_t tx_len, comm_status;
    uint16_t rx_len;
    
    switch (g_nfc_state) {
        case NFC_STATE_INIT:
            break;
        
        case NFC_STATE_LPCD_CONFIG:
            /* Entry into Low Power Card Detection mode */
            pcd_lpcd_config_start();
            /* Record timestamp for interval check */
            g_nfc_timer = Get_TickCount(); 
            g_nfc_state = NFC_STATE_LPCD_WAITING;
            break;

        case NFC_STATE_LPCD_WAITING:
            /* Non-blocking: Check if LPCD inactive time has elapsed */
            if ((Get_TickCount() - g_nfc_timer) >= (lpcd.inact_ms + 5)) {
                /* Poll the TagDetIrq flag in BC45B4522 */
                if (pcd_lpcd_check() == TRUE) {
                    g_nfc_state = NFC_STATE_TAG_DETECTED;
                } else {
                    /* No tag found, refresh timer and stay in LPCD */
                    g_nfc_timer = Get_TickCount();
                }
            }
            break;

        case NFC_STATE_TAG_DETECTED:
            /* Tag detected, switch to full power ISO14443A mode */
            BC45_ConfigurationNoOfffield(CONFIG_14443A);
            pcd_antenna_on(); /* Enable RF field for communication */
            g_nfc_state = NFC_STATE_TAG_SCANNING;
            break;

        case NFC_STATE_TAG_SCANNING:
            /* Execute the actual UID scanning process (REQA + Anticollision) */
            g_scan_resp = com_reqa(PICC_REQALL);
            
            if (g_scan_resp == MI_OK) {
                g_nfc_state = NFC_STATE_TAG_REPORT;
            } else {
                /* Failed scan or false trigger, return to LPCD after cooling */
                g_nfc_timer = Get_TickCount();
                g_nfc_state = NFC_STATE_TAG_LEAVE_WAIT;
            }
            break;

        case NFC_STATE_TAG_REPORT:
        {
            /* 1. Convert HEX UID to String */
            g_task_bufLen = g_tag_info.uid_length;
            UIDTypeA_BytesToChar(g_tag_info.serial_num, (uint8_t*)g_task_buffer, &g_task_bufLen);
            
            /* 2. Calculate Wiegand Values */
            /* Assumption: MIFARE UID uses Little-Endian, so we set reverse to 1 */
            uint8_t REVERSE_BYTE_ORDER = 1; 
            
            uint32_t wg34_num = Calculate_Wiegand34(g_tag_info.serial_num, REVERSE_BYTE_ORDER);
            uint8_t wg26_fc;
            uint16_t wg26_cn;
            Calculate_Wiegand26(g_tag_info.serial_num, REVERSE_BYTE_ORDER, &wg26_fc, &wg26_cn);
            uint32_t wg26_8digit = (wg26_fc * 100000) + wg26_cn;
            
            /* 3. Print to local console with Wiegand Info */
            printf("\r\n=========================================\r\n");
            printf(" [NFC TAG DETECTED] \r\n");
            printf(" -> HEX UID : %s\r\n", g_task_buffer);
            printf(" -> WG34    : %010u\r\n", wg34_num);
            printf(" -> WG26    : FC:%03u, CN:%05u (8-Digit: %08u)\r\n", wg26_fc, wg26_cn, wg26_8digit);
            printf("=========================================\r\n");
#if 0            
            /* 4. Publish to MQTT via Task_wifi.c interface (JSON Format) */
            char mqtt_payload[128];
            snprintf(mqtt_payload, sizeof(mqtt_payload), 
                     "{\"uid\":\"%s\",\"wg34\":%010u,\"wg26\":\"%03u,%05u\"}", 
                     g_task_buffer, wg34_num, wg26_fc, wg26_cn);
            
            //Task_Wifi_Publish_MQTT("nfc_reader/report", mqtt_payload);

            /* --- CAN Transmission Section --- */
            /* Construct the CAN message containing the raw UID */
            can_msg_t nfc_can_msg;
            nfc_can_msg.id = 0x1A2;          /* Standard ID for NFC Report, adjust as needed */
            nfc_can_msg.is_extended = false; /* Use Standard ID format */
            
            /* CAN payload maximum is 8 bytes. Restrict DLC if UID is unusually long */
            nfc_can_msg.dlc = (g_tag_info.uid_length > 8) ? 8 : g_tag_info.uid_length;
            
            /* Copy the raw UID bytes directly into the CAN payload */
            for (int i = 0; i < nfc_can_msg.dlc; i++) {
                nfc_can_msg.data[i] = g_tag_info.serial_num[i];
            }
            
            /* Pad the remaining bytes with zero (good practice to avoid garbage data) */
            for (int i = nfc_can_msg.dlc; i < 8; i++) {
                nfc_can_msg.data[i] = 0x00;
            }
            
            /* Safely push the message to the CAN TX queue (Non-blocking) */
            task_can_send_msg(&nfc_can_msg);
#endif

#if 0
            /* --- RS232 Transmission Section --- */
            /* Create a concise string for RS232 transmission.
             * Note: Ensure this length does not exceed TASK_RS232_MAX_PAYLOAD_SIZE (default 64) 
             */
            char rs232_payload[64];
            int rs232_len = snprintf(rs232_payload, sizeof(rs232_payload), "RS232 => %s,WG34:%010u\r\n", g_task_buffer, wg34_num);
            
            if (rs232_len > 0) {
                /* Push the formatted string into the RS232 task's transmission queue */
                task_rs232_send((const uint8_t *)rs232_payload, (uint16_t)rs232_len);
            }
#endif

#if 0            
            /* --- RS485 Transmission Section --- */
            /* Create a concise string for RS485 transmission.
             * Note: Ensure this length does not exceed TASK_RS485_MAX_PAYLOAD_SIZE (default 64) 
             */
            char rs485_payload[64];
            int rs485_len = snprintf(rs485_payload, sizeof(rs485_payload), "RS485:%s,WG34:%010u\r\n", g_task_buffer, wg34_num);
            
            if (rs485_len > 0) {
                /* Push the formatted string into the RS485 task's transmission queue */
                task_rs485_send((const uint8_t *)rs485_payload, (uint16_t)rs485_len);
            }
            
 #endif           
            /* 5. Update Peripheral Status */
            Task_OLED_Set_Dashboard(g_task_buffer);
            Task_Buzzer_SetMode(BUZZER_MODE_BEEP_ONCE);
            
            g_nfc_timer = Get_TickCount();
            g_nfc_state = NFC_STATE_TAG_LEAVE_WAIT;
            break;
        }

        case NFC_STATE_TAG_LEAVE_WAIT:
            /* Cooling period to prevent repeated UART printing */
            if ((Get_TickCount() - g_nfc_timer) >= 500) {
                g_nfc_state = NFC_STATE_LPCD_CONFIG;
            }
            break;
            
        /* -------------------------------------------------------------
         * Image Transmission Flow (Triggered by Button)
         * ------------------------------------------------------------- */
         
        /* Step 1: Turn off antenna to reset tag, begin 500ms wait */
        case NFC_STATE_TRANS_INIT:           
            pcd_antenna_off();
            g_nfc_timer = Get_TickCount(); /* Utilize your original Get_TickCount architecture */
            g_nfc_state = NFC_STATE_TRANS_WAIT_ANT_OFF;
            break;

        /* Step 2: 500ms elapsed. Turn on antenna, begin 1000ms wait */
        case NFC_STATE_TRANS_WAIT_ANT_OFF:
            if ((Get_TickCount() - g_nfc_timer) >= 500) {
                pcd_antenna_on();
                g_nfc_timer = Get_TickCount();
                g_nfc_state = NFC_STATE_TRANS_WAIT_ANT_ON;
            }
            break;

        /* Step 3: 1000ms elapsed. Wake up Tag, reset counters */
        case NFC_STATE_TRANS_WAIT_ANT_ON:
            if ((Get_TickCount() - g_nfc_timer) >= 1000) {
                FD_Config();          /* Activate / Wake-up Tag */
                chunk_idx = 0;        /* Reset chunk counter (0 to 194) */
                data_offset = 0;      /* Reset payload reading offset */
                g_nfc_state = NFC_STATE_TRANS_SEND_CHUNK;
                //PB2 = 0;
                //PB3 = 0;
            }
            break;

        /* Step 4: Pack 64 bytes of payload, compute CRC, and send */
        case NFC_STATE_TRANS_SEND_CHUNK:
            //PB2 = ~PB2;
            /* Fill fixed header */
            tx_buffer[0] = 0xA6;
            tx_buffer[1] = 0xF0;
            tx_buffer[2] = 0xFF;
            
            /* Fill 64-byte image data payload */
            if (chunk_idx < TOTAL_CHUNKS) {
                for (int i = 0; i < CHUNK_SIZE; i++) {
                    if(gu8_image_flag == 0) {
                        tx_buffer[3 + i] = image_data[data_offset + i];
                    }
                    else
                    {
                        tx_buffer[3 + i] = image_data_1[data_offset + i];
                    }                  
//                    tx_buffer[3 + i] = i;
//                    if((i%16) == 0)
//                    {
//                        printf("\n");
//                    }
//                    printf("%02X ", tx_buffer[3 + i]);
                }
            }
            //printf("\n");
            
            /* Calculate CRC16 for the first 67 bytes */
            crc_val = crc16_2(tx_buffer, 67, 0x6363); 
            tx_buffer[67] = (uint8_t)(crc_val & 0xFF);         /* LSB */
            tx_buffer[68] = (uint8_t)((crc_val >> 8) & 0xFF);  /* MSB */
            
            tx_len = 69;
            rx_len = 10;
            
            /* Transmit packet over RF (Hardware CRC disabled) */
            pcd_com_transceive_no_crc(tx_buffer, tx_len, 10, rx_buffer, &rx_len);
            //pcd_com_transceive_crc(tx_buffer, tx_len, 10, rx_buffer, &rx_len);
            
            /* Restart timer for ACK timeout detection */
            g_nfc_timer = Get_TickCount(); 
            g_nfc_state = NFC_STATE_TRANS_WAIT_ACK;
            break;

        /* Step 5: Wait 10ms for Tag's ACK (0x21). Handle success/error */
        case NFC_STATE_TRANS_WAIT_ACK:
            if ((Get_TickCount() - g_nfc_timer) >= 50) { // Holtek Run @ 8 Mhz
            //if ((Get_TickCount() - g_nfc_timer) >= 15) { // Holtek Run @ 16 Mhz
                //printf("T:%d\n", g_nfc_timer);
                //PB3 = ~PB3;
                
                comm_status = pcd_read(0xED, &ReceivedData[0]); 
                //printf("Sts: %02X, Rec:%02X \n", comm_status, ReceivedData[2]);
                
                if(comm_status != 0x00)
                {
                    g_nfc_timer = Get_TickCount(); 
                    g_nfc_state = NFC_STATE_TRANS_FAIL;                
                }
                else if(ReceivedData[2] == 0x21)
                {                    
                    chunk_idx++;
                    data_offset += CHUNK_SIZE;
                    
                    if (chunk_idx >= TOTAL_CHUNKS) {
                        drv_led_set_on(&g_leds[LED_ID_G], 100);
                        //drv_led_set_blink(&g_leds[LED_ID_G], 100, 100);
                        /* Transfer successfully completed -> Play single beep */
                        Task_Buzzer_SetMode(BUZZER_MODE_BEEP_ONCE); 
                        g_nfc_timer = Get_TickCount();
                        g_nfc_state = NFC_STATE_INIT; 
                        //g_nfc_state = NFC_STATE_TRANS_WAIT_EPD_UPDATE;
                        
                    } else {
                        /* Proceed to send the next chunk */
                        g_nfc_state = NFC_STATE_TRANS_SEND_CHUNK; 
                    }                
                }
                else
                {
                    g_nfc_timer = Get_TickCount(); 
                }
                #if 0
                /* 0x21 indicates the Tag successfully received the chunk */
                if (ReceivedData[2] == 0x21) {
                    drv_led_set_blink(&g_leds[LED_ID_G], 100, 100);
                    chunk_idx++;
                    data_offset += CHUNK_SIZE;
                    
                    if (chunk_idx >= TOTAL_CHUNKS) {
                        /* Transfer successfully completed -> Play single beep */
                        Task_Buzzer_SetMode(BUZZER_MODE_BEEP_ONCE); 
                        g_nfc_timer = Get_TickCount();
                        //g_nfc_state = NFC_STATE_INIT; 
                        g_nfc_state = NFC_STATE_TRANS_WAIT_EPD_UPDATE;
                        
                    } else {
                        /* Proceed to send the next chunk */
                        g_nfc_state = NFC_STATE_TRANS_SEND_CHUNK; 
                    }
                } 
                /* Communication Error or Incorrect Response */
                else if (comm_status != 0x00) {
                    g_nfc_timer = Get_TickCount(); 
                    g_nfc_state = NFC_STATE_TRANS_FAIL;
                }
                /* Timeout / Tag didn't respond yet, reset wait timer */
                else {
                    g_nfc_timer = Get_TickCount(); 
                }
                #endif
            }
            break;
            
        case NFC_STATE_TRANS_WAIT_EPD_UPDATE:
            if ((Get_TickCount() - g_nfc_timer) >= 10000) { 
                gu8_image_flag = ~gu8_image_flag;
                drv_led_set_on(&g_leds[LED_ID_G], 100);  
                drv_led_set_off(&g_leds[LED_ID_R]);  
                g_nfc_state = NFC_STATE_TRANS_INIT;
            }
            break;
            
        case NFC_STATE_TRANS_FAIL:
            if ((Get_TickCount() - g_nfc_timer) >= 2000) { 
                /* Emit double beep to alert user about the failure */
                Task_Buzzer_SetMode(BUZZER_MODE_BEEP_DOUBLE); 
                drv_led_set_off(&g_leds[LED_ID_G]);
                drv_led_set_on(&g_leds[LED_ID_R], 100); 
                g_nfc_state = NFC_STATE_TRANS_INIT;
            }            
            break;

        default:
            //g_nfc_state = NFC_STATE_INIT;
            g_nfc_state = NFC_STATE_TRANS_INIT;
            break;
    }
}