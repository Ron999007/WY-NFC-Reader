/***************************************************************************//**
 * @file     task_usb_vcom.c
 * @brief    USB Virtual COM Port Task (Using utils_ringbuffer)
 ******************************************************************************/
#include "MyApplication.h"

/* =======================================================================
 * Internal Ring Buffers for USB I/O
 * ======================================================================= */
#define VCOM_RX_BUF_SIZE 512  /* Data from PC to MCU */
#define VCOM_TX_BUF_SIZE 1024 /* Data from MCU to PC */

/* Memory pools for ring buffers */
static uint8_t s_rx_mem[VCOM_RX_BUF_SIZE];
static uint8_t s_tx_mem[VCOM_TX_BUF_SIZE];

/* Ring buffer instance handlers */
static Utils_RB_t s_rx_rb;
static Utils_RB_t s_tx_rb;

/* Flag to track if hardware is currently transmitting */
static volatile uint32_t gu32TxSize = 0; 

/* =======================================================================
 * Global Variables (Required by CDC and hsusbd.c)
 * ======================================================================= */
STR_VCOM_LINE_CODING gLineCoding = {115200, 0, 0, 8}; 
uint16_t gCtrlSignal = 0;     

/* External variables from Nuvoton USB Library */
//extern uint32_t volatile g_hsusbd_DmaDone;
//extern uint8_t g_hsusbd_ShortPacket;
//extern uint32_t g_hsusbd_CtrlInSize;
//extern uint8_t g_hsusbd_CtrlZero;

/* =======================================================================
 * Initialization Functions
 * ======================================================================= */
void VCOM_InitForHighSpeed(void)
{
    HSUSBD_SetEpBufAddr(EPA, EPA_BUF_BASE, EPA_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPA, EPA_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPA, BULK_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_IN);

    HSUSBD_SetEpBufAddr(EPB, EPB_BUF_BASE, EPB_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPB, EPB_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPB, BULK_OUT_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(EPB, HSUSBD_EPINTEN_RXPKIEN_Msk | HSUSBD_EPINTEN_SHORTRXIEN_Msk);

    HSUSBD_SetEpBufAddr(EPC, EPC_BUF_BASE, EPC_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPC, EPC_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPC, INT_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_IN);
}

void VCOM_InitForFullSpeed(void)
{
    HSUSBD_SetEpBufAddr(EPA, EPA_BUF_BASE, EPA_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPA, EPA_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPA, BULK_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_IN);

    HSUSBD_SetEpBufAddr(EPB, EPB_BUF_BASE, EPB_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPB, EPB_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPB, BULK_OUT_EP_NUM, HSUSBD_EP_CFG_TYPE_BULK, HSUSBD_EP_CFG_DIR_OUT);
    HSUSBD_ENABLE_EP_INT(EPB, HSUSBD_EPINTEN_RXPKIEN_Msk | HSUSBD_EPINTEN_SHORTRXIEN_Msk);

    HSUSBD_SetEpBufAddr(EPC, EPC_BUF_BASE, EPC_BUF_LEN);
    HSUSBD_SET_MAX_PAYLOAD(EPC, EPC_OTHER_MAX_PKT_SIZE);
    HSUSBD_ConfigEp(EPC, INT_IN_EP_NUM, HSUSBD_EP_CFG_TYPE_INT, HSUSBD_EP_CFG_DIR_IN);
}

void VCOM_Init(void)
{
    HSUSBD_ENABLE_USB_INT(HSUSBD_GINTEN_USBIEN_Msk|HSUSBD_GINTEN_CEPIEN_Msk|HSUSBD_GINTEN_EPAIEN_Msk|HSUSBD_GINTEN_EPBIEN_Msk|HSUSBD_GINTEN_EPCIEN_Msk);
    HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_DMADONEIEN_Msk|HSUSBD_BUSINTEN_RESUMEIEN_Msk|HSUSBD_BUSINTEN_RSTIEN_Msk|HSUSBD_BUSINTEN_VBUSDETIEN_Msk);
    HSUSBD_SET_ADDR(0);

    HSUSBD_SetEpBufAddr(CEP, CEP_BUF_BASE, CEP_BUF_LEN);
    HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk|HSUSBD_CEPINTEN_STSDONEIEN_Msk);

    VCOM_InitForHighSpeed();
}

void Task_USB_VCOM_Init(void)
{
    /* Initialize ring buffers using utility functions */
    Utils_RB_Init(&s_rx_rb, s_rx_mem, VCOM_RX_BUF_SIZE, sizeof(uint8_t));
    Utils_RB_Init(&s_tx_rb, s_tx_mem, VCOM_TX_BUF_SIZE, sizeof(uint8_t));
    gu32TxSize = 0;

    HSUSBD_Open(&gsHSInfo, VCOM_ClassRequest, NULL);
    VCOM_Init();
    NVIC_EnableIRQ(USBD20_IRQn);
}

/* =======================================================================
 * Exported I/O APIs for Console Dependency Injection
 * ======================================================================= */
uint32_t VCOM_Read(uint8_t *buf, uint32_t max_len)
{
    uint32_t count = 0;
    
    NVIC_DisableIRQ(USBD20_IRQn);
    /* Pop data safely using utility API */
    while ((count < max_len) && !Utils_RB_IsEmpty(&s_rx_rb)) 
    {
        Utils_RB_Pop(&s_rx_rb, &buf[count]);
        count++;
    }
    NVIC_EnableIRQ(USBD20_IRQn);
    
    return count;
}

uint32_t VCOM_Write(uint8_t *buf, uint32_t len)
{
    uint32_t count = 0;

    /* Drop data if USB is not connected */
    if (!HSUSBD_IS_ATTACHED()) return 0;

    while (count < len) 
    {
        NVIC_DisableIRQ(USBD20_IRQn);
        
        /* 1. Push data to TX Ring Buffer */
        if (!Utils_RB_IsFull(&s_tx_rb)) 
        {
            Utils_RB_Push(&s_tx_rb, &buf[count]);
            count++;
        }
        
        /* 2. If hardware is idle, kickstart the transmission */
        uint32_t pending_bytes = Utils_RB_GetCount(&s_tx_rb);
        if (gu32TxSize == 0 && pending_bytes > 0) 
        {
            uint32_t send_len = (pending_bytes > EPA_MAX_PKT_SIZE) ? EPA_MAX_PKT_SIZE : pending_bytes;
            for (uint32_t i = 0; i < send_len; i++) 
            {
                uint8_t tx_data;
                Utils_RB_Pop(&s_tx_rb, &tx_data);
                HSUSBD->EP[EPA].EPDAT_BYTE = tx_data;
            }
            gu32TxSize = send_len; /* Flag hardware as busy */
            
            HSUSBD->EP[EPA].EPRSPCTL = HSUSBD_EP_RSPCTL_SHORTTXEN;
            HSUSBD->EP[EPA].EPTXCNT = send_len;
            HSUSBD_ENABLE_EP_INT(EPA, HSUSBD_EPINTEN_INTKIEN_Msk);
        }
        
        NVIC_EnableIRQ(USBD20_IRQn);
    }
    return count;
}

/* =======================================================================
 * CDC Class Request
 * ======================================================================= */
void VCOM_ClassRequest(void)
{
    if (gUsbCmd.bmRequestType & 0x80)
    {
        /* Device to host */
        if (gUsbCmd.bRequest == GET_LINE_CODE) {
            if ((gUsbCmd.wIndex & 0xff) == 0)
                HSUSBD_PrepareCtrlIn((uint8_t *)&gLineCoding, 7);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
        } else {
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
        }
    }
    else
    {
        /* Host to device */
        if (gUsbCmd.bRequest == SET_CONTROL_LINE_STATE) {
            if ((gUsbCmd.wIndex & 0xff) == 0) gCtrlSignal = gUsbCmd.wValue;
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
        } 
        else if (gUsbCmd.bRequest == SET_LINE_CODE) {
            if ((gUsbCmd.wIndex & 0xff) == 0) HSUSBD_CtrlOut((uint8_t *)&gLineCoding, 7);
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
        } 
        else {
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_STALLEN_Msk);
        }
    }
}

/* =======================================================================
 * Hardare Interrupt Service Routine 
 * ======================================================================= */
void USBD20_IRQHandler(void)
{
    __IO uint32_t IrqStL, IrqSt;

    IrqStL = HSUSBD->GINTSTS & HSUSBD->GINTEN;

    if (!IrqStL) return;

    /* USB interrupt */
    if (IrqStL & HSUSBD_GINTSTS_USBIF_Msk)
    {
        IrqSt = HSUSBD->BUSINTSTS & HSUSBD->BUSINTEN;

        if (IrqSt & HSUSBD_BUSINTSTS_SOFIF_Msk)
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_SOFIF_Msk);

        if (IrqSt & HSUSBD_BUSINTSTS_RSTIF_Msk)
        {
            HSUSBD_SwReset();
            HSUSBD_ResetDMA();
            HSUSBD->EP[EPA].EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;
            HSUSBD->EP[EPB].EPRSPCTL = HSUSBD_EPRSPCTL_FLUSH_Msk;

            /* Reset Ring Buffers on Bus Reset */
            Utils_RB_Init(&s_rx_rb, s_rx_mem, VCOM_RX_BUF_SIZE, sizeof(uint8_t));
            Utils_RB_Init(&s_tx_rb, s_tx_mem, VCOM_TX_BUF_SIZE, sizeof(uint8_t));
            gu32TxSize = 0;

            if (HSUSBD->OPER & 0x04)  
                VCOM_InitForHighSpeed();
            else                    
                VCOM_InitForFullSpeed();
                
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            HSUSBD_SET_ADDR(0);
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk|HSUSBD_BUSINTEN_RESUMEIEN_Msk|HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_RSTIF_Msk);
            HSUSBD_CLR_CEP_INT_FLAG(0x1ffc);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_RESUMEIF_Msk)
        {
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk|HSUSBD_BUSINTEN_SUSPENDIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_RESUMEIF_Msk);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_SUSPENDIF_Msk)
        {
            HSUSBD_ENABLE_BUS_INT(HSUSBD_BUSINTEN_RSTIEN_Msk | HSUSBD_BUSINTEN_RESUMEIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_SUSPENDIF_Msk);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_HISPDIF_Msk)
        {
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_HISPDIF_Msk);
        }

        if (IrqSt & HSUSBD_BUSINTSTS_DMADONEIF_Msk)
        {
            g_hsusbd_DmaDone = 1;
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_DMADONEIF_Msk);

            if (!(HSUSBD->DMACTL & HSUSBD_DMACTL_DMARD_Msk))
            {
                HSUSBD_ENABLE_EP_INT(EPB, HSUSBD_EPINTEN_RXPKIEN_Msk);
            }

            if (HSUSBD->DMACTL & HSUSBD_DMACTL_DMARD_Msk)
            {
                if (g_hsusbd_ShortPacket == 1)
                {
                    HSUSBD->EP[EPA].EPRSPCTL = (HSUSBD->EP[EPA].EPRSPCTL & 0x10) | HSUSBD_EP_RSPCTL_SHORTTXEN;
                    g_hsusbd_ShortPacket = 0;
                }
            }
        }

        if (IrqSt & HSUSBD_BUSINTSTS_PHYCLKVLDIF_Msk)
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_PHYCLKVLDIF_Msk);

        if (IrqSt & HSUSBD_BUSINTSTS_VBUSDETIF_Msk)
        {
            if (HSUSBD_IS_ATTACHED())
            {
                /* USB Plug In */
                HSUSBD_ENABLE_USB();
                /* Hot-plug start support */
                HSUSBD_Start(); 
            }
            else
            {
                /* USB Un-plug */
                HSUSBD_DISABLE_USB();
            }
            HSUSBD_CLR_BUS_INT_FLAG(HSUSBD_BUSINTSTS_VBUSDETIF_Msk);
        }
    }

    if (IrqStL & HSUSBD_GINTSTS_CEPIF_Msk)
    {
        IrqSt = HSUSBD->CEPINTSTS & HSUSBD->CEPINTEN;

        if (IrqSt & HSUSBD_CEPINTSTS_SETUPTKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_SETUPTKIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_SETUPPKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_SETUPPKIF_Msk);
            HSUSBD_ProcessSetupPacket();
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_OUTTKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_OUTTKIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_INTKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
            if (!(IrqSt & HSUSBD_CEPINTSTS_STSDONEIF_Msk)) {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_TXPKIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_TXPKIEN_Msk);
                HSUSBD_CtrlIn();
            } else {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_TXPKIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_TXPKIEN_Msk|HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            }
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_PINGIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_PINGIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_TXPKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            if (g_hsusbd_CtrlInSize) {
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_INTKIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_INTKIEN_Msk);
            } else {
                if (g_hsusbd_CtrlZero == 1)
                    HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_ZEROLEN);
                HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
                HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk|HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            }
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_TXPKIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_RXPKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_RXPKIF_Msk);
            HSUSBD_SET_CEP_STATE(HSUSBD_CEPCTL_NAKCLR);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk|HSUSBD_CEPINTEN_STSDONEIEN_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_NAKIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_NAKIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_STALLIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STALLIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_ERRIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_ERRIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_STSDONEIF_Msk) {
            HSUSBD_UpdateDeviceState();
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_STSDONEIF_Msk);
            HSUSBD_ENABLE_CEP_INT(HSUSBD_CEPINTEN_SETUPPKIEN_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_BUFFULLIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_BUFFULLIF_Msk);
            return;
        }
        if (IrqSt & HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk) {
            HSUSBD_CLR_CEP_INT_FLAG(HSUSBD_CEPINTSTS_BUFEMPTYIF_Msk);
            return;
        }
    }

    /* bulk in (EPA) */
    if (IrqStL & HSUSBD_GINTSTS_EPAIF_Msk)
    {
        IrqSt = HSUSBD->EP[EPA].EPINTSTS & HSUSBD->EP[EPA].EPINTEN;

        uint32_t pending_bytes = Utils_RB_GetCount(&s_tx_rb);
        
        if (pending_bytes > 0) 
        {
            uint32_t send_len = (pending_bytes > EPA_MAX_PKT_SIZE) ? EPA_MAX_PKT_SIZE : pending_bytes;
            for (uint32_t i = 0; i < send_len; i++) 
            {
                uint8_t tx_data;
                Utils_RB_Pop(&s_tx_rb, &tx_data);
                HSUSBD->EP[EPA].EPDAT_BYTE = tx_data;
            }
            gu32TxSize = send_len; 
            
            HSUSBD->EP[EPA].EPRSPCTL = HSUSBD_EP_RSPCTL_SHORTTXEN;
            HSUSBD->EP[EPA].EPTXCNT = send_len;
        } 
        else 
        {
            gu32TxSize = 0; 
            HSUSBD_ENABLE_EP_INT(EPA, 0); 
        }

        HSUSBD_CLR_EP_INT_FLAG(EPA, IrqSt);
    }
    
    /* bulk out (EPB) */
    if (IrqStL & HSUSBD_GINTSTS_EPBIF_Msk)
    {
        int volatile i;
        IrqSt = HSUSBD->EP[EPB].EPINTSTS & HSUSBD->EP[EPB].EPINTEN;
        
        uint32_t rx_cnt = HSUSBD->EP[EPB].EPDATCNT & 0xffff;

        for (i = 0; i < rx_cnt; i++) 
        {
            if (!Utils_RB_IsFull(&s_rx_rb)) 
            {
                uint8_t rx_data = HSUSBD->EP[EPB].EPDAT_BYTE;
                Utils_RB_Push(&s_rx_rb, &rx_data);
            } 
            else 
            {
                /* Buffer full: drop data but clear hardware FIFO */
                volatile uint8_t dummy = HSUSBD->EP[EPB].EPDAT_BYTE;
                (void)dummy;
            }
        }

        HSUSBD_CLR_EP_INT_FLAG(EPB, IrqSt);
    }

    if (IrqStL & HSUSBD_GINTSTS_EPCIF_Msk) { IrqSt = HSUSBD->EP[EPC].EPINTSTS & HSUSBD->EP[EPC].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPC, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPDIF_Msk) { IrqSt = HSUSBD->EP[EPD].EPINTSTS & HSUSBD->EP[EPD].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPD, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPEIF_Msk) { IrqSt = HSUSBD->EP[EPE].EPINTSTS & HSUSBD->EP[EPE].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPE, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPFIF_Msk) { IrqSt = HSUSBD->EP[EPF].EPINTSTS & HSUSBD->EP[EPF].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPF, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPGIF_Msk) { IrqSt = HSUSBD->EP[EPG].EPINTSTS & HSUSBD->EP[EPG].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPG, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPHIF_Msk) { IrqSt = HSUSBD->EP[EPH].EPINTSTS & HSUSBD->EP[EPH].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPH, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPIIF_Msk) { IrqSt = HSUSBD->EP[EPI].EPINTSTS & HSUSBD->EP[EPI].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPI, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPJIF_Msk) { IrqSt = HSUSBD->EP[EPJ].EPINTSTS & HSUSBD->EP[EPJ].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPJ, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPKIF_Msk) { IrqSt = HSUSBD->EP[EPK].EPINTSTS & HSUSBD->EP[EPK].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPK, IrqSt); }
    if (IrqStL & HSUSBD_GINTSTS_EPLIF_Msk) { IrqSt = HSUSBD->EP[EPL].EPINTSTS & HSUSBD->EP[EPL].EPINTEN; HSUSBD_CLR_EP_INT_FLAG(EPL, IrqSt); }
}

