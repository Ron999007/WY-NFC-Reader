/**
 ****************************************************************
 * @file bc45b45ss.h
 *
 * @brief 
 *
 * @author crystal@holtek.com
 *
 ****************************************************************
 */ 
#ifndef BC45_H
#define BC45_H
//#include "main.h"
//#include "define.h"
#include "macro_utils.h"

/*
 * DEFINES Registers Address
 ****************************************************************
 */
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01    
#define     ComIEnReg             0x02    
#define     DivIEnReg             0x03    
#define     ComIrqReg             0x04    
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    
#define     Status1Reg            0x07    
#define     Status2Reg            0x08    
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxASKReg              0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MfTxReg	              0x1C
#define     MfRxReg               0x1D
#define     TypeBReg              0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsPReg              0x28
#define     ModGsPReg             0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
	#define		PAGE4				0x5e
	#define		PAGE5				0xae
	#define		PAGE6				0x5a
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     SpecialReg	  		  0x3F

// PAGE 4
#define		AGCCfgReg			0x35
#define		RxCKReg				0x3A
#define		RxBandReg			0x3B
#define		LPCDReg				0x3C
#define		WUPeriodReg 		0x3D
#define		SwingsCntReg		0x3E

// PAGE 6
#define		CalibReg			0x33
#define 	ADCRefReg_LPCD		0x36
#define		CWGsN_LPCD			0x38
#define 	CWGsP_LPCD			0x39

/*
 * DEFINES Registers bits
 ****************************************************************
 */
#define TxIEn 		BIT6
#define RxIEn 		BIT5
#define IdleIEn		BIT4
#define ErrIEn		BIT1
#define TimerIEn	BIT0
#define TxIRq 		BIT6
#define RxIRq 		BIT5
#define IdleIRq		BIT4
#define ErrIRq		BIT1
#define TimerIRq	BIT0

#define CollErr		BIT3
#define CrcErr		BIT2
#define ParityErr	BIT1
#define ProtocolErr BIT0

#define CollPos		(BIT0|BIT1|BIT2|BIT3|BIT4)

#define RxAlign		(BIT4|BIT5|BIT6)
#define TxLastBits	(BIT0|BIT1|BIT2)
/**
 * PCD command
 ****************************************************************
 */
#define PCD_IDLE              	0x00       	//cancel current command
#define PCD_AUTHENT           	0x0E       	//authenticate key
#define PCD_RECEIVE           	0x08       	//receive data
#define PCD_TRANSMIT          	0x04       	//transmit data
#define PCD_TRANSCEIVE        	0x0C       	//receive & transmit data
#define PCD_RESETPHASE        	0x0F       	//reset
#define PCD_CALCCRC           	0x03       	//calculate CRC
#define PCD_CMD_MASK		  	0x0F		//command mask


/** 
 * Mifare Error Codes
 * Each function returns a status value, which corresponds to 
 * the mifare error
 * codes. 
 ****************************************************************
 */ 
#define MI_OK							0
#define MI_CHK_OK						0
#define MI_CRC_ZERO						0
#define _SUCCESS_						0

#define MI_CRC_NOTZERO					1

#define MI_NOTAGERR						0xFF//(-1)
#define MI_CHK_FAILED                   0xFF//(-1)
#define MI_CRCERR						0xFE//(-2)
#define MI_CHK_COMPERR					0xFE//(-2)
#define MI_EMPTY						0xFD//(-3)
#define MI_AUTHERR						0xFC//(-4)
#define MI_PARITYERR					0xFB//(-5)
#define MI_CODEERR						0xFA//(-6)
#define MI_SERNRERR						0xF8//(-8)
#define MI_KEYERR						0xF7//(-9)
#define MI_NOTAUTHERR                   0xF6//(-10)
#define MI_BITCOUNTERR                  0xF5//(-11)
#define MI_BYTECOUNTERR					0xF4//(-12)
#define MI_IDLE							0xF3//(-13)
#define MI_TRANSERR						0xF2//(-14)
#define MI_WRITEERR						0xF1//(-15)
#define MI_INCRERR						0xF0//(-16)
#define MI_DECRERR						0xEF//(-17)
#define MI_READERR						0xEE//(-18)
#define MI_OVFLERR						0xED//(-19)
#define MI_POLLING						0xEC//(-20)
#define MI_FRAMINGERR                   0xEB//(-21)
#define MI_ACCESSERR                    0xEA//(-22)
#define MI_UNKNOWN_COMMAND				0xE9//(-23)
#define MI_COLLERR						0xE8//(-24)
#define MI_RESETERR						0xE7//(-25)
#define MI_INITERR						0xE6//(-25)
#define MI_INTERFACEERR                 0xE5//(-26)
#define MI_ACCESSTIMEOUT                0xE4//(-27)
#define MI_NOBITWISEANTICOLL			0xE3//(-28)
#define MI_QUIT							0xE1//(-30)
#define MI_INTEGRITY_ERR				0xDD//(-35) //crc/parity/protocol
#define MI_RECBUF_OVERFLOW              0xCE//(-50) 
#define MI_SENDBYTENR                   0xCD//(-51)	
#define MI_SENDBUF_OVERFLOW             0xCB//(-53)
#define MI_BAUDRATE_NOT_SUPPORTED       0xCA//(-54)
#define MI_SAME_BAUDRATE_REQUIRED       0xC9//(-55)
#define MI_WRONG_PARAMETER_VALUE        0xC4//(-60)
#define MI_BREAK						0x9D//(-99)
#define MI_NY_IMPLEMENTED				0x9C//(-100)
#define MI_NO_MFRC						0x9B//(-101)
#define MI_MFRC_NOTAUTH					0x9A//(-102)
#define MI_WRONG_DES_MODE				0x99//(-103)
#define MI_HOST_AUTH_FAILED				0x98//(-104)
#define MI_WRONG_LOAD_MODE				0x96//(-106)
#define MI_WRONG_DESKEY					0x95//(-107)
#define MI_MKLOAD_FAILED				0x94//(-108)
#define MI_FIFOERR						0x93//(-109)
#define MI_WRONG_ADDR					0x92//(-110)
#define MI_DESKEYLOAD_FAILED			0x91//(-111)
#define MI_WRONG_SEL_CNT				0x8E//(-114)
#define MI_WRONG_TEST_MODE				0x8B//(-117)
#define MI_TEST_FAILED					0x8A//(-118)
#define MI_TOC_ERROR					0x89//(-119)
#define MI_COMM_ABORT					0x88//(-120)
#define MI_INVALID_BASE					0x87//(-121)
#define MI_MFRC_RESET					0x86//(-122)
#define MI_WRONG_VALUE					0x85//(-123)
#define MI_VALERR						0x84//(-124)
#define MI_COM_ERR                      0x83//(-125)
#define PROTOCOL_ERR					0x82//(-126)

///user error
#define USER_ERROR						0x81//(-127)
#define FAILED							0x81//(-127)
#define _ERROR_							0x81//(-127)


#define	READ_REG_CTRL	0x80
#define FIFO_SIZE		64
#define FSD 			256 	//Frame Size for proximity coupling Device
#define FSDI 8 					//Frame Size for proximity coupling Device

#define FWI_DEFAULT	4			//
typedef struct pcd_param
{
	uint8_t u27VA;
	uint8_t u28VA;
	uint8_t u29VA;

	uint8_t u27VB;
	uint8_t u28VB;
	uint8_t u29VB;
}pcd_param;
extern pcd_param pcd;

typedef struct lpcd_param
{
    uint8_t     power_mode;
    uint8_t     delta;
    uint8_t     skip_times;
    uint8_t     detect_us;
    uint32_t    inact_ms;
	
	uint8_t		u33V;
	uint8_t		u36V;
	uint8_t		u38V;
	uint8_t		u39V;
}lpcd_param;
extern lpcd_param lpcd;

typedef struct transceive_buffer{
	uint8_t mf_command;
	uint16_t mf_length;
	uint8_t mf_data[FSD];
}transceive_buffer;
extern transceive_buffer mf_com_data;

#if(INT_USE_CHECK_REG)
//check interrupt by polling interrupt flag (reg 0x07 irq bit)
#define INT_PIN 	(read_reg(Status1Reg) & 0x10)
#else
//check interrupt by gpio interrupt
#define INT_PIN      (BC45_IRQ_Port->DINR & GPIO_PIN_7)
#endif
/*****************************Add for emv*********************************/

void pcd_init(void);
void pcd_reset(void);
void change_baudrate(void);
uint8_t pcd_config(uint8_t tag_type);


void write_reg(uint8_t addr, uint8_t val);
uint8_t read_reg(uint8_t addr);
int pcd_com_transceive(struct transceive_buffer *pi);
int pcd_com_transceive_crc(uint8_t *tx_buffer, uint16_t tx_len, uint8_t fwi, uint8_t *rx_buffer, uint16_t *rx_len);
int pcd_com_transceive_no_crc(uint8_t *tx_buffer, uint16_t tx_len, uint8_t fwi, uint8_t *rx_buffer, uint16_t *rx_len);

void set_bit_mask(uint8_t reg, uint8_t mask);  
void clear_bit_mask(uint8_t reg,uint8_t mask);  
void pcd_set_tmo(uint8_t fwi);
void pcd_set_rate(uint8_t rate, uint8_t type);
void pcd_antenna_on(void);
void pcd_antenna_off(void);
void pcd_antenna_reset(void);
void page45_lock(void);
void page4_unlock(void);
void page5_unlock(void);

void pcd_delay_sfgi(uint8_t sfgi);
void pcd_lpcd_end(void);
void pcd_lpcd_config_start(void);
uint8_t pcd_lpcd_check(void);

#endif

