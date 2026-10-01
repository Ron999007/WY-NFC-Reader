#include "MyApplication.h"
#include "task_eeprom.h"
#include <stdio.h>
#include <string.h>

/* ====================================================================
 * Firmware Version Definitions & Variables
 * ==================================================================== */

#define FW_VERSION_MAJOR      0
#define FW_VERSION_MINOR      0
#define FW_VERSION_PATCH      0

#define FW_MAGIC_WORD         0x5A5A
#define EEPROM_ADDR_FW_INFO   0x0000

/**
 * @brief Firmware version data structure (Exactly 8 bytes).
 */
typedef struct {
    uint16_t magic;      /* Identifier to check if EEPROM is formatted */
    uint8_t  major;      /* Major version number */
    uint8_t  minor;      /* Minor version number */
    uint8_t  patch;      /* Patch version number */
    uint8_t  reserved[3];/* Reserved for alignment and future use */
} fw_info_t;

/* Global variables for Async Buffer Storage */
static fw_info_t g_eeprom_fw_info; 
static const fw_info_t g_current_fw_info = {
    .magic = FW_MAGIC_WORD,
    .major = FW_VERSION_MAJOR,
    .minor = FW_VERSION_MINOR,
    .patch = FW_VERSION_PATCH,
    .reserved = {0}
};

/* State flag to indicate completion of startup check */
static volatile bool g_fw_check_done = false;


/* ====================================================================
 * Firmware Version Check Callbacks
 * ==================================================================== */

/**
 * @brief Callback triggered when the EEPROM write operation finishes.
 */
static void on_fw_write_done(bool success, void *user_data) {
    if (success) {
        printf("[FW] Version successfully updated in EEPROM.\n");
    } else {
        printf("[FW] Error: Failed to write version to EEPROM!\n");
    }
    
    /* Mark the startup check as completely finished */
    g_fw_check_done = true;
}

/**
 * @brief Callback triggered when the EEPROM read operation finishes.
 */
static void on_fw_read_done(bool success, void *user_data) {
    if (!success) {
        printf("[FW] Error: Failed to read from EEPROM!\n");
        /* Prevent the system from hanging forever if EEPROM is physically disconnected */
        g_fw_check_done = true; 
        return;
    }

    bool need_update = false;

    /* Check Condition 1: Is it empty (e.g., 0xFF) or corrupted? */
    if (g_eeprom_fw_info.magic != FW_MAGIC_WORD) {
        printf("[FW] EEPROM is empty or uninitialized. Needs formatting.\n");
        need_update = true;
    } 
    /* Check Condition 2: Does the version differ? */
    else if (g_eeprom_fw_info.major != FW_VERSION_MAJOR ||
             g_eeprom_fw_info.minor != FW_VERSION_MINOR ||
             g_eeprom_fw_info.patch != FW_VERSION_PATCH) {
                 
        printf("[FW] Version mismatch detected! Updating %d.%d.%d -> %d.%d.%d\n", 
                g_eeprom_fw_info.major, g_eeprom_fw_info.minor, g_eeprom_fw_info.patch,
                FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);
        need_update = true;
    }

    /* Execute logic based on the check results */
    if (need_update) {
        /* Initiate an async write. Buffer is static const, so it is safe to pass its pointer. */
        task_eeprom_write(EEPROM_ADDR_FW_INFO, (const uint8_t *)&g_current_fw_info, sizeof(fw_info_t), on_fw_write_done, NULL);
    } else {
        printf("[FW] Version matches current firmware. No update needed.\n");
        /* Mark the startup check as completely finished */
        g_fw_check_done = true;
    }
}

/* ====================================================================
 * Main Application
 * ==================================================================== */

void MyApplication_Init(void)
{
    task_led_init();
//    Task_RGB_LED_Init();
    Task_Button_Init();
    Task_Console_Init();
//    Task_Snor_Flash_Init();
//    Task_Ymodem_Init();
//    Task_wifi_Init();
    Task_NFC_Reader_Init();
    Task_OLED_Init();
//    Task_SDCard_Init();
    Task_Buzzer_Init();
//    Task_DHT11_Init();
//    task_rs232_init();
//    task_rs485_init();
//    task_can_init(CAN1, 500000);
    
    /* Initialize the EEPROM task first */
    task_eeprom_init();
    
    /* Start the async firmware version check */
    g_fw_check_done = false;
    printf("[FW] Starting version check...\n");
    task_eeprom_read(EEPROM_ADDR_FW_INFO, (uint8_t *)&g_eeprom_fw_info, sizeof(fw_info_t), on_fw_read_done, NULL);
}

void MyApplication_Run(void)
{
    /* * 1. System-critical background tasks that must always run.
     * This ensures the EEPROM I2C state machine and callbacks keep processing.
     */
    task_eeprom();

    /* * 2. Wait for the EEPROM firmware version check to complete.
     * Other tasks will be blocked from running until the version is verified or updated.
     */
    if (g_fw_check_done) 
    {
        
        /* 3. Run normal application tasks only after system initialization is fully complete */
        task_led();         // 10 ms
//        Task_RGB_LED();     // 10 ms
        Task_Button();      // 10 ms
        Task_Console();
//        Task_Snor_Flash();
//        Task_Ymodem();  
//        Task_wifi();  
        Task_NFC_Reader();   
        Task_OLED();        
//        Task_SDCard();
        Task_Buzzer();      // 1 ms
//        Task_DHT11();
//        task_rs232();
//        task_rs485();
//        task_can();
    }
}

void Client_Handler(void)
{
    //LED_ProcessAll();
}