#include "MyApplication.h"

/* ====================================================================
 * Private Variables & Definitions
 * ==================================================================== */

/* Global Variables for Combo Logic */
static volatile uint32_t g_locked_combo_keys = 0;

/* Combo Key Definitions */
#define MASK_BTN_1       (1UL << 0)
#define MASK_BTN_2       (1UL << 1)
#define COMBO_1_2        (MASK_BTN_1 | MASK_BTN_2)

/* Hardware layer read function */
static uint8_t HW_Button_ReadPin(void* port, uint32_t pin) 
{
    volatile uint32_t *reg = (volatile uint32_t *)pin;
    return (uint8_t)(*reg);
}

/* Button Configuration Array */
static button_t my_buttons[] = {
//   ID         PORT          PIN                           ACTIVE
  { .id = 1,   .port = NULL, .pin = (uint32_t)&BTN_SW,     .active_level = 0 }
};

#define BTN_COUNT (sizeof(my_buttons) / sizeof(my_buttons[0]))

/* ====================================================================
 * Task API Implementation
 * ==================================================================== */

void Task_Button_Init(void)
{
    Button_Init(my_buttons, BTN_COUNT, HW_Button_ReadPin);
}

void Task_Button(void)
{
    static uint32_t u32_TickCnt_Temp = 0;
    static uint8_t  combo_handled = 0;  
    btn_msg_t msg;                      
       
    /* 1. Timer Logic: Execute Button_Ticks() every 10ms */
    if (Get_TickCount() - u32_TickCnt_Temp >= 10)
    {
        Button_Ticks();
        u32_TickCnt_Temp += 10;
    }

    /* 2. Combo Key Polling Logic (Continuous Check) */
    uint32_t current_mask = Button_Get_State_Mask();
    
    if ((current_mask & COMBO_1_2) == COMBO_1_2) 
    {
        if (!combo_handled) {
            printf("SUPER COMBO: 2 Keys Pressed Together!\n");
            combo_handled = 1;
            g_locked_combo_keys |= COMBO_1_2; /* Lock the combo keys */
        }
    } 
    else 
    {
        combo_handled = 0; /* Reset flag when keys are released */
    }

    /* 3. Event Queue Processing Logic */
    while (Button_Read_Event(&msg)) 
    {
        uint32_t my_mask = (1UL << (msg.id - 1));

        /* Release Event: Unlock the key */
        if (msg.event == BTN_EVENT_RELEASE) {
            g_locked_combo_keys &= ~my_mask;
            continue; 
        }

        /* Protection: Ignore single-key events if locked by a combo */
        if (g_locked_combo_keys & my_mask) {
            continue; 
        }

        /* Application Logic Based on Events */
        if (msg.event == BTN_EVENT_SHORT_CLICK) 
        {
            printf("Button %d Short Clicked!\n", msg.id);
            
            //Task_NFC_TriggerImageTransfer();
            if (Task_Image_Flag_Get() == 0)
            {
                Task_NFC_TriggerImageTransfer(image_data);
            } 
            else 
            {
                Task_NFC_TriggerImageTransfer(image_data_1);
            }
#if 0            
            /* Control LEDs defined in task_led.h */
            switch (msg.id) 
            {
                case 1:
                    if (drv_led_get_current_mode(&g_leds[LED_ID_R]) == LED_MODE_OFF) {
                        drv_led_set_on(&g_leds[LED_ID_R], 100);
                    } else if (drv_led_get_current_mode(&g_leds[LED_ID_R]) == LED_MODE_ON) {
                        drv_led_set_blink(&g_leds[LED_ID_R], 200, 10);
                    } else if (drv_led_get_current_mode(&g_leds[LED_ID_R]) == LED_MODE_BLINK) {
                        drv_led_set_breathe(&g_leds[LED_ID_R], 50, 100);
                    } else {
                        drv_led_set_off(&g_leds[LED_ID_R]);
                    }              
                    break;
                    
                case 2:
                    if (drv_led_get_current_mode(&g_leds[LED_ID_Y]) == LED_MODE_OFF) {
                        drv_led_set_on(&g_leds[LED_ID_Y], 100);
                    } else if (drv_led_get_current_mode(&g_leds[LED_ID_Y]) == LED_MODE_ON) {
                        drv_led_set_blink(&g_leds[LED_ID_Y], 200, 10);
                    } else if (drv_led_get_current_mode(&g_leds[LED_ID_Y]) == LED_MODE_BLINK) {
                        drv_led_set_breathe(&g_leds[LED_ID_Y], 50, 100);
                    } else {
                        drv_led_set_off(&g_leds[LED_ID_Y]);
                    }                     
                    break;
                    
                default:
                    break;
            }
#endif
        } 
        else if (msg.event == BTN_EVENT_LONG_PRESS) 
        {
            printf("Button %d Long Pressed!\n", msg.id);
        }
    }
}