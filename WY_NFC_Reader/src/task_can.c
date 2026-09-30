/**
 * @file    task_can.c
 * @brief   CAN Bus Task Manager Implementation.
 */

#include "MyApplication.h"

/* ==================================================================== */
/* ??? DEBUG SWITCH: Set to 1 to enable periodic loopback test messages  */
/* ==================================================================== */
#define ENABLE_CAN_TEST_MODE 1

static periph_can_obj_t   hw_can_obj;
static can_io_interface_t sys_can_intf;
static can_io_interface_t *p_can_bus = NULL;
static can_task_state_t    current_state = CAN_STATE_INIT;

static can_msg_t rx_storage[CAN_RX_QUEUE_CAPACITY];
static can_msg_t tx_storage[CAN_TX_QUEUE_CAPACITY];
static Utils_RB_t rx_queue;
static Utils_RB_t tx_queue;

static void task_can_rx_callback(const can_msg_t *msg) {
    if (msg == NULL) return;
    Utils_RB_Push(&rx_queue, msg);
}

void task_can_init(CAN_T *can_base, uint32_t baudrate) {
    if (can_base == NULL) return;
    
    /* Set PF6 as CAN1 Term EN pin */
    SYS->GPF_MFPL &= ~SYS_GPF_MFPL_PF6MFP_Msk;
    SYS->GPF_MFPL |= SYS_GPF_MFPL_PF6MFP_GPIO;
    GPIO_SetMode(PF, (BIT6), GPIO_MODE_OUTPUT);
    CAN1_TERM_EN = 1;

    periph_can_init(&hw_can_obj, can_base, baudrate);
    periph_can_bind_interface(&hw_can_obj, &sys_can_intf);
    p_can_bus = &sys_can_intf;

    Utils_RB_Init(&rx_queue, rx_storage, CAN_RX_QUEUE_CAPACITY, sizeof(can_msg_t));
    Utils_RB_Init(&tx_queue, tx_storage, CAN_TX_QUEUE_CAPACITY, sizeof(can_msg_t));

    p_can_bus->set_rx_callback(p_can_bus->user_data, task_can_rx_callback);
    current_state = CAN_STATE_IDLE;
}

bool task_can_send_msg(const can_msg_t *msg) {
    if (msg == NULL) return false;
    return Utils_RB_Push(&tx_queue, msg);
}

void task_can(void) {
    can_msg_t current_msg;

#if 0
    static uint32_t u32_TickCnt_Temp = 0;
    static bool     is_first_run = true;

    if (is_first_run) {
        u32_TickCnt_Temp = Get_TickCount();
        is_first_run = false;
    }

    if (Get_TickCount() - u32_TickCnt_Temp >= 1000) {
        can_msg_t test_msg;
        test_msg.id = 0x1A2;
        test_msg.dlc = 8;
        test_msg.is_extended = false;
        for (int i = 0; i < 8; i++) {
            test_msg.data[i] = i * 0x11;
        }
        
        task_can_send_msg(&test_msg);
        u32_TickCnt_Temp = Get_TickCount(); 
    }
#endif

    switch (current_state) {
        case CAN_STATE_INIT:
            break;

        case CAN_STATE_IDLE:
            //printf("1:%u \r\n", Get_TickCount());
            if (!Utils_RB_IsEmpty(&rx_queue)) {
                current_state = CAN_STATE_PROCESS_RX;
            } 
            else if (!Utils_RB_IsEmpty(&tx_queue)) {
                current_state = CAN_STATE_PROCESS_TX;
            }
            //printf("2:%u \r\n", Get_TickCount());
            break;

        case CAN_STATE_PROCESS_RX:
            if (Utils_RB_Pop(&rx_queue, &current_msg)) {
                
#if ENABLE_CAN_TEST_MODE
                printf("[%u ms] [CAN RX] ID: 0x%03X | Data: %02X %02X %02X %02X\n", 
                       Get_TickCount(), current_msg.id, current_msg.data[0], 
                       current_msg.data[1], current_msg.data[2], current_msg.data[3]);
#endif
            }
            current_state = CAN_STATE_IDLE;
            break;

        case CAN_STATE_PROCESS_TX:
            if (Utils_RB_Pop(&tx_queue, &current_msg)) {
                if (p_can_bus != NULL && p_can_bus->transmit != NULL) {
                    if (p_can_bus->transmit(p_can_bus->user_data, &current_msg)) {
#if ENABLE_CAN_TEST_MODE
                        printf("[%u ms] [CAN TX] ID: 0x%03X | Data: %02X %02X %02X %02X\n", 
                               Get_TickCount(), current_msg.id, current_msg.data[0], 
                               current_msg.data[1], current_msg.data[2], current_msg.data[3]);
#endif
                    }
                }
            }
            current_state = CAN_STATE_IDLE;
            break;

        case CAN_STATE_ERROR:
            current_state = CAN_STATE_IDLE;
            break;

        default:
            current_state = CAN_STATE_INIT;
            break;
    }
}