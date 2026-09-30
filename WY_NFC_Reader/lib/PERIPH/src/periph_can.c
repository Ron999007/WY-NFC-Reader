/**
 * @file    periph_can.c
 * @brief   Specific CAN Peripheral Driver (Non-Blocking Direct Register Version).
 * @details Bypasses BSP while-loops for ultimate real-time performance. 
 * Configured for Silent + Loopback testing.
 */

#include "periph_can.h"
#include <string.h>

static bool hw_can_transmit_wrapper(void *user_data, const can_msg_t *msg);
static void hw_can_set_rx_callback_wrapper(void *user_data, can_rx_cb_t cb);

static periph_can_obj_t *p_can0_obj = NULL;
static periph_can_obj_t *p_can1_obj = NULL;

void periph_can_init(periph_can_obj_t *obj, CAN_T *can_base, uint32_t baudrate) {
    if (obj == NULL || can_base == NULL) return;

    obj->can_base = can_base;
    obj->app_cb = NULL;

    SYS_UnlockReg();

    if (can_base == CAN0) {
        obj->irq_num = CAN0_IRQn;
        p_can0_obj = obj;
        CLK_EnableModuleClock(CAN0_MODULE);
        SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk)) |
                        (SYS_GPA_MFPL_PA4MFP_CAN0_RXD | SYS_GPA_MFPL_PA5MFP_CAN0_TXD);
    } 
    else if (can_base == CAN1) {
        obj->irq_num = CAN1_IRQn;
        p_can1_obj = obj;
        CLK_EnableModuleClock(CAN1_MODULE);
        SYS->GPC_MFPL = (SYS->GPC_MFPL & ~(SYS_GPC_MFPL_PC2MFP_Msk | SYS_GPC_MFPL_PC3MFP_Msk)) |
                        (SYS_GPC_MFPL_PC2MFP_CAN1_RXD | SYS_GPC_MFPL_PC3MFP_CAN1_TXD);
    }

    SYS_LockReg();

    CAN_Open(obj->can_base, baudrate, CAN_NORMAL_MODE);
    
    /* Enable general TX/RX interrupts only, preventing error storm */
    CAN_EnableInt(obj->can_base, CAN_CON_IE_Msk);
    
    /* Configure to receive all messages (Mask = 0) */
    CAN_SetRxMsgAndMsk(obj->can_base, 0, CAN_STD_ID, 0, 0); 
    
    NVIC_EnableIRQ((IRQn_Type)obj->irq_num);

    /* ==================================================================== */
    /* DEBUG TEST BLOCK: Silent + Loopback Mode                             */
    /* Completely isolates external physical pin states, preventing         */
    /* Bus-Off errors due to disconnected CAN transceivers.                 */
    /* ==================================================================== */
    //CAN_EnterTestMode(obj->can_base, CAN_TEST_LBACK_Msk | CAN_TEST_SILENT_Msk);
}

void periph_can_bind_interface(periph_can_obj_t *obj, can_io_interface_t *out_interface) {
    if (obj == NULL || out_interface == NULL) return;
    out_interface->user_data       = (void *)obj;
    out_interface->transmit        = hw_can_transmit_wrapper;
    out_interface->set_rx_callback = hw_can_set_rx_callback_wrapper;
}

/**
 * @brief Ultra-fast non-blocking transmission (Using IF0)
 */
static bool hw_can_transmit_wrapper(void *user_data, const can_msg_t *msg) {
    periph_can_obj_t *obj = (periph_can_obj_t *)user_data;
    CAN_T *tCAN = obj->can_base;

    /* Fail-safe: If IF0 is busy, abort immediately and let the Queue retry next time. 
       Never block the CPU! */
    if (tCAN->IF[0].CREQ & CAN_IF_CREQ_BUSY_Msk) {
        return false; 
    }

    tCAN->IF[0].CMASK = CAN_IF_CMASK_WRRD_Msk | CAN_IF_CMASK_MASK_Msk | CAN_IF_CMASK_ARB_Msk |
                        CAN_IF_CMASK_CONTROL_Msk | CAN_IF_CMASK_DATAA_Msk | CAN_IF_CMASK_DATAB_Msk;

    if (msg->is_extended) {
        tCAN->IF[0].ARB1 = msg->id & 0xFFFF;
        tCAN->IF[0].ARB2 = ((msg->id >> 16) & 0x1FFF) | CAN_IF_ARB2_DIR_Msk | CAN_IF_ARB2_XTD_Msk | CAN_IF_ARB2_MSGVAL_Msk;
    } else {
        tCAN->IF[0].ARB1 = 0;
        tCAN->IF[0].ARB2 = ((msg->id & 0x7FF) << 2) | CAN_IF_ARB2_DIR_Msk | CAN_IF_ARB2_MSGVAL_Msk;
    }

    tCAN->IF[0].DAT_A1 = (msg->data[1] << 8) | msg->data[0];
    tCAN->IF[0].DAT_A2 = (msg->data[3] << 8) | msg->data[2];
    tCAN->IF[0].DAT_B1 = (msg->data[5] << 8) | msg->data[4];
    tCAN->IF[0].DAT_B2 = (msg->data[7] << 8) | msg->data[6];

    tCAN->IF[0].MCON = CAN_IF_MCON_NEWDAT_Msk | msg->dlc | CAN_IF_MCON_TXIE_Msk | CAN_IF_MCON_EOB_Msk;

    /* Fire and forget: Trigger hardware transfer and CPU leaves immediately */
    tCAN->IF[0].CMASK |= CAN_IF_CMASK_TXRQSTNEWDAT_Msk;
    tCAN->IF[0].CREQ = 1 + 31;

    return true;
}

static void hw_can_set_rx_callback_wrapper(void *user_data, can_rx_cb_t cb) {
    if (user_data == NULL) return;
    periph_can_obj_t *obj = (periph_can_obj_t *)user_data;
    obj->app_cb = cb;
}

/**
 * @brief Dedicated interrupt reception and clearing (Using IF1)
 */
static void internal_can_irq_handler(periph_can_obj_t *obj) {
    if (obj == NULL || obj->can_base == NULL) return;
    CAN_T *tCAN = obj->can_base;

    uint32_t status = tCAN->IIDR;
    
    if (status == 0x8000) {
        CAN_GET_INT_STATUS(tCAN); 
        return;
    }
    
    if (status >= 1 && status <= 32) {
        uint8_t msg_obj_num = status - 1;

        if (msg_obj_num == 0) { /* Object 0 triggers RX interrupt */
            if ((tCAN->IF[1].CREQ & CAN_IF_CREQ_BUSY_Msk) == 0) {
                tCAN->IF[1].CMASK = CAN_IF_CMASK_MASK_Msk | CAN_IF_CMASK_ARB_Msk | 
                                    CAN_IF_CMASK_CONTROL_Msk | CAN_IF_CMASK_CLRINTPND_Msk | 
                                    CAN_IF_CMASK_DATAA_Msk | CAN_IF_CMASK_DATAB_Msk;
                tCAN->IF[1].CREQ = 1 + 0; 
                
                /* Extremely short safety wait (usually a few clocks) to ensure 
                   hardware has finished transferring data from RAM to IF1 */
                uint32_t timeout = 500;
                while ((tCAN->IF[1].CREQ & CAN_IF_CREQ_BUSY_Msk) && timeout-- > 0);
                
                if (timeout > 0 && obj->app_cb != NULL) {
                    can_msg_t rx_msg;
                    rx_msg.is_extended = (tCAN->IF[1].ARB2 & CAN_IF_ARB2_XTD_Msk) ? true : false;
                    rx_msg.id = rx_msg.is_extended ? 
                                (((tCAN->IF[1].ARB2 & 0x1FFF) << 16) | tCAN->IF[1].ARB1) : 
                                ((tCAN->IF[1].ARB2 & CAN_IF_ARB2_ID_Msk) >> 2);
                    rx_msg.dlc = tCAN->IF[1].MCON & CAN_IF_MCON_DLC_Msk;
                    rx_msg.data[0] = tCAN->IF[1].DAT_A1 & 0xFF;
                    rx_msg.data[1] = (tCAN->IF[1].DAT_A1 >> 8) & 0xFF;
                    rx_msg.data[2] = tCAN->IF[1].DAT_A2 & 0xFF;
                    rx_msg.data[3] = (tCAN->IF[1].DAT_A2 >> 8) & 0xFF;
                    rx_msg.data[4] = tCAN->IF[1].DAT_B1 & 0xFF;
                    rx_msg.data[5] = (tCAN->IF[1].DAT_B1 >> 8) & 0xFF;
                    rx_msg.data[6] = tCAN->IF[1].DAT_B2 & 0xFF;
                    rx_msg.data[7] = (tCAN->IF[1].DAT_B2 >> 8) & 0xFF;
                    
                    obj->app_cb(&rx_msg);
                }
            }
        } 
        else if (msg_obj_num == 31) { /* Object 31 triggers TX complete interrupt */
            if ((tCAN->IF[1].CREQ & CAN_IF_CREQ_BUSY_Msk) == 0) {
                /* Clear interrupt pending flag only */
                tCAN->IF[1].CMASK = CAN_IF_CMASK_CLRINTPND_Msk;
                tCAN->IF[1].CREQ = 1 + 31;
            }
        }
    }
}

void CAN0_IRQHandler(void) 
{ 
    internal_can_irq_handler(p_can0_obj); 
}

void CAN1_IRQHandler(void) 
{ 
    internal_can_irq_handler(p_can1_obj); 
}