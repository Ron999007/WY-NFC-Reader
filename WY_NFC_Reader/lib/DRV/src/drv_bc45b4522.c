#include "macro_utils.h"
#include "periph_spi.h"
#include "iso14443a.h"
#include "iso14443b.h"
#include "iso14443_4.h"
//#include "card_detect.h"
#include "drv_bc45b4522.h"

#define READ_REG_CTRL  	0x80
#define TP_FWT_302us	2048
#define TP_dFWT	192

#define MAX_RX_REQ_WAIT_MS	5000	//command waiting time out 100ms

transceive_buffer mf_com_data;
lpcd_param lpcd;
pcd_param pcd;
	
void pcd_init()
{
	pcd.u27VA = 0xF8;
	pcd.u28VA = 0x3F;
	pcd.u29VA = 0x16;
	
	pcd.u27VB = 0xF8;
	pcd.u28VB = 0x20;
	pcd.u29VB = 0x16;
	
	//Ron CardDetect_Init();
    
	//lpcd.power_mode = DSLEEPMODE;
	lpcd.delta = 7;
	lpcd.inact_ms = 100;
	lpcd.detect_us = 6;
	lpcd.skip_times = 0;
	
	lpcd.u33V = 0xa0;
	lpcd.u36V = 0x80;
	lpcd.u38V = 0xf0;
	lpcd.u39V = 0x1f;
}

/**
 ****************************************************************
 * @brief pcd_config() 
 *
 * Configure NFC-A/NFC-B mode
 *
 * @param: uint8_t type   
 * @return: 
 * @retval: 
 ****************************************************************
 */
uint8_t pcd_config(uint8_t type)
{
	//pcd_antenna_off();
	
	if ('A' == type)
	{
#if (NFC_DEBUG)
		printf("\r\nPCD CONFIG A");
#endif
	    clear_bit_mask(Status2Reg, BIT3);	//0x08, MFCrypto1On=0
		clear_bit_mask(ComIEnReg, BIT7); 	//0x02, IRQ not inverted
        write_reg(ModeReg,0x3D);			//0x11, CRC seed:6363
		write_reg(RxSelReg, 0x88);			//0x17, Rx waiting time after Tx 

    	//write_reg(RFCfgReg, 0x48);		//0x26, 0x38, 33db	//RX max gain, max sensing distance		
		//write_reg(RxModeReg, 0x00);		//0x13 ,Rx framing A, disable crc, 106k, nfc-a
		//write_reg(ControlReg, 0x00);
		
		//Tx
		write_reg(GsNReg, pcd.u27VA);			//0x27, field strength, n-driver, no modulation
		write_reg(CWGsPReg, pcd.u28VA); 			//0x28, field strength, p-driver, no moudlation
		write_reg(ModGsPReg, pcd.u29VA); 		//0x29, Tx field strength, p-driver, moudlation
		write_reg(TxASKReg, 0x40);			//0x15, bit6=1, 100%ASK typeA		
		write_reg(TxModeReg, 0x00);			//12 //Tx Framing A, disable crc, 106k, nfc-a
		write_reg(RxModeReg, 0x00);			//

		//page4
		write_reg(VersionReg, PAGE4);
		write_reg(RxCKReg, 0x18);			//0x3a, automatic noise filter bandwidth
		write_reg(AGCCfgReg, 0xED);			//0x35, 11:automatic gain enable, 10, saturation point, 11, 
											//		11,initial value, 36db, 01, gainstep=4db

		write_reg(VersionReg, 0x00);	//lock		
	}
	else if ('B' == type)
	{
#if (NFC_DEBUG)
		printf("\r\nPCD CONFIG B");
#endif
	    //clear_bit_mask(Status2Reg, BIT3);	//0x08, MFCrypto1On=0
		write_reg(Status2Reg, 0x00);	
		clear_bit_mask(ComIEnReg, BIT7); 	//0x02, IRQ not inverted
        write_reg(ModeReg,0x3F);			//0x11, CRC seed:FFFF  
		write_reg(RxSelReg, 0x88);			//0x17, Rx waiting time after Tx 
		write_reg(ControlReg, 0x00);

		//Tx
		write_reg(GsNReg, pcd.u27VB);			//0x27, field strength, n-driver, no modulation, F8h
		write_reg(CWGsPReg, pcd.u28VB); 		//0x28, field strength, p-driver, no moudlation, 20h
		write_reg(ModGsPReg, pcd.u29VB); 		//0x29, field strength, p-driver, moudlation, 16h
		
		write_reg(AutoTestReg, 0x00);		//
		write_reg(TxASKReg, 0x00); 			//0x15, bit6=0, not 100%ASK typeB
		write_reg(TypeBReg, 0x10);			//RX: No EOF/EOS required, IN EMV need //0xD3, EGT=0
											//forum tester cannot catch the signal when EGT = 0x13
		write_reg(TxModeReg, 0x83); 		//Tx Framing B
		write_reg(RxModeReg, 0x83); 		//Rx framing B
		write_reg(BitFramingReg, 0x00);		//TxLastBits=0
		
		//page 4
		write_reg(VersionReg, PAGE4);		//page4
		write_reg(AGCCfgReg, 0xED);			//0x35, 11:automatic gain enable, 10, saturation point, 11, 
											//		11,initial value, 36db, 01, gainstep=4db
		write_reg(0x3b, 0x25);				//default value	
		write_reg(VersionReg, 0x00);
		
	}
	else
	{
		return USER_ERROR;
	}

	//pcd_antenna_on();

	return MI_OK;
}


int pcd_com_transceive_no_crc(uint8_t *tx_buffer, uint16_t tx_len, uint8_t fwi, uint8_t *rx_buffer, uint16_t *rx_len)
{
	int status;
    transceive_buffer  *pi;
    pi = &mf_com_data;
	
	write_reg(BitFramingReg,0x00); 		//Tx last bits = 0, rx align = 0
	clear_bit_mask(TxModeReg, BIT7); 	//disable tx CRC
	clear_bit_mask(RxModeReg, BIT7); 	//disable rx CRC
	pcd_set_tmo(fwi);	//302us * (1<<fwi)

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = tx_len;
	memcpy(mf_com_data.mf_data,tx_buffer,tx_len);

	status = pcd_com_transceive(pi);
	*rx_len = mf_com_data.mf_length/8;
	memcpy(rx_buffer, mf_com_data.mf_data,*rx_len);

	return status;
}

int pcd_com_transceive_crc(uint8_t *tx_buffer, uint16_t tx_len, uint8_t fwi, uint8_t *rx_buffer, uint16_t *rx_len)
{
	volatile int status;
    transceive_buffer  *pi;
    pi = &mf_com_data;

	write_reg(BitFramingReg,0x00); 		//Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 		//enable tx crc
	set_bit_mask(RxModeReg, BIT7); 		//enable rx crc
	pcd_set_tmo(fwi);	//302us * (1<<fwi)

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = tx_len;
	memcpy(mf_com_data.mf_data,tx_buffer,tx_len);
	
	status = pcd_com_transceive(pi);
	
	*rx_len = mf_com_data.mf_length/8;
	memcpy(rx_buffer, mf_com_data.mf_data,*rx_len);
	
	return status;
}

/**
 ****************************************************************
 * @brief pcd_com_transceive() 
 *
 * communication with 14443 card
 *
 * @param: pi->mf_command = chip command
 * @param: pi->mf_length  = length of data to send
 * @param: pi->mf_data[]  = data to send 
 * @return: status = MI_OK, successful
 * @retval: pi->mf_length  = bit count of data received
 * @retval: pi->mf_data[]  = data received
 ****************************************************************
 */
int pcd_com_transceive(transceive_buffer *pi)
{
	int status = MI_OK;
	uint8_t  recebyte;
	uint8_t irq_en;		//add volatile for compiler v6
	uint8_t wait_for;	//add volatile for compiler v6
	uint8_t last_bits;
	uint8_t j;
	uint8_t val;
	uint8_t err;

	uint8_t irq_inv;
	uint16_t len_rest;
	uint8_t len;
	uint8_t WATER_LEVEL;
	
	len = 0;
	len_rest = 0;
	err = 0;
	recebyte = 0;
	irq_en = 0;
	wait_for = 0;

	//GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_0);
	switch (pi->mf_command)
	{
		case PCD_IDLE:
			irq_en   = 0x00;
			wait_for = 0x00;
			break;
		case PCD_AUTHENT:
			irq_en = IdleIEn | TimerIEn;
	     	wait_for = IdleIRq;
			break;
		case PCD_RECEIVE:
	     	irq_en   = RxIEn | IdleIEn;
			wait_for = RxIRq;
			recebyte=1;
			break;	 
		case PCD_TRANSMIT:
			irq_en   = TxIEn | IdleIEn;
			wait_for = TxIRq;
			break;
	  	case PCD_TRANSCEIVE:
		 	irq_en = RxIEn | IdleIEn | TimerIEn | TxIEn;
	     	wait_for = RxIRq;
	     	recebyte=1;
	     	break;
	  	default:
	     	pi->mf_command = MI_UNKNOWN_COMMAND;
	     	break;
	}
   
	WATER_LEVEL = read_reg(WaterLevelReg);
		
	//the sending length is not allowed to be 0
	if (pi->mf_command != MI_UNKNOWN_COMMAND 
		&& (((pi->mf_command == PCD_TRANSCEIVE || pi->mf_command == PCD_TRANSMIT) && pi->mf_length > 0)
		|| (pi->mf_command != PCD_TRANSCEIVE && pi->mf_command != PCD_TRANSMIT))
		)
	{	
		#if (NFC_DEBUG)
		printf("\r\n1 comirq=%02x,ien=%02x,INT= %d \r\n", (uint16_t)read_reg(ComIrqReg), (uint16_t)read_reg(ComIEnReg), (uint16_t)INT_PIN);
		#endif
		write_reg(CommandReg, PCD_IDLE);
		
		irq_inv = read_reg(ComIEnReg) & BIT7;
		write_reg(ComIEnReg, irq_inv | irq_en | BIT0);	//enable timer interrupt
		write_reg(ComIrqReg, 0x7F); 					//Clear INT
		write_reg(DivIrqReg, 0x7F); 					//Clear INT

		//Flush Fifo
		set_bit_mask(FIFOLevelReg, BIT7);
		
		if (pi->mf_command == PCD_TRANSCEIVE || pi->mf_command == PCD_TRANSMIT || pi->mf_command == PCD_AUTHENT)
		{
			#if (NFC_DEBUG)
			printf(" PCD_tx:");
			for (j = 0; j < pi->mf_length; j++)
			{
				printf("%02X ", (uint16_t)pi->mf_data[j]);
			}
			printf("l=%d", pi->mf_length);
			printf("\r\n");
			#endif


			//printf("C-APDU -> : ");
			len_rest = pi->mf_length;
			if (len_rest >= FIFO_SIZE)
			{
				len = FIFO_SIZE;
			}else
			{
				len = len_rest;
			}
			
			for (j = 0; j < len; j++)
			{
				write_reg(FIFODataReg, pi->mf_data[j]);
			}
			len_rest -= len;//Rest bytes
			if (len_rest != 0)
			{
				write_reg(ComIrqReg, BIT2); 	// clear LoAlertIRq
				set_bit_mask(ComIEnReg, BIT2);	// enable LoAlertIRq
			}

			write_reg(CommandReg, pi->mf_command);
			if (pi->mf_command == PCD_TRANSCEIVE)
		    {    
				set_bit_mask(BitFramingReg,0x80);  
			}
		
			while (len_rest != 0)
			{
				while(INT_PIN == 0);//Wait LoAlertIRq		
				if (len_rest > (FIFO_SIZE - WATER_LEVEL))
				{
					len = FIFO_SIZE - WATER_LEVEL;
				}
				else
				{
					len = len_rest;
				}
				for (j = 0; j < len; j++)
				{
					write_reg(FIFODataReg, pi->mf_data[pi->mf_length - len_rest + j]);
				}

				write_reg(ComIrqReg, BIT2);		//clear interrupt flag after writing fifo
			
				#if (NFC_DEBUG)
				printf("\r\n8 comirq=%02x,ien=%02x,INT= %d \r\n", (uint16_t)read_reg(ComIrqReg), (uint16_t)read_reg(ComIEnReg), (uint16_t)INT_PIN);
				#endif
				len_rest -= len;//Rest bytes
				if (len_rest == 0)
				{
					clear_bit_mask(ComIEnReg, BIT2);// disable LoAlertIRq
					#if (NFC_DEBUG)
					printf("\r\n9 comirq=%02x,ien=%02x,INT= %d \r\n", (uint16_t)read_reg(ComIrqReg), (uint16_t)read_reg(ComIEnReg), (uint16_t)INT_PIN);
					#endif
				}
			}

			//Wait TxIRq
			while (INT_PIN == 0);
			
			val = read_reg(ComIrqReg);
			if (val & TxIRq)
			{
				write_reg(ComIrqReg, TxIRq);
				if(pi->mf_command == PCD_TRANSMIT)
				{
					return 0;
				}
			}
		}
		if (PCD_RECEIVE == pi->mf_command)
		{
			write_reg(CommandReg, PCD_RECEIVE);	//*
			clear_bit_mask(TModeReg,BIT7);		//*
			set_bit_mask(ControlReg, BIT6);// TStartNow
		}

		len_rest = 0; 					// bytes received
		write_reg(ComIrqReg, BIT3); 	// clear HoAlertIRq
		set_bit_mask(ComIEnReg, BIT3); 	// enable HoAlertIRq

		while(status != MI_NOTAGERR)
		{
			//wait for command to complete
			while(INT_PIN == 0);
		
			while(1)
			{
				while(0 == INT_PIN);
				val = read_reg(ComIrqReg);
				if ((val & BIT3) && !(val & BIT5))
				{
					if (len_rest + FIFO_SIZE - WATER_LEVEL > 255)
					{
						#if (NFC_DEBUG)
						printf("AF RX_LEN > 255B\r\n");
						#endif
						break;
					}
					for (j = 0; j <FIFO_SIZE - WATER_LEVEL; j++)
					{
						pi->mf_data[len_rest + j] = read_reg(FIFODataReg);
					}
					write_reg(ComIrqReg, BIT3);//clear interrupt flag after reading fifo
					len_rest += FIFO_SIZE - WATER_LEVEL; 
				}
				else
				{
					clear_bit_mask(ComIEnReg, BIT3);//disable HoAlertIRq
					break;
				}			
			}


			val = read_reg(ComIrqReg);
			#if (NFC_DEBUG)
			printf(" INT:fflvl=%d,rxlst=%02x ,ien=%02x,cirq=%02x\r\n", (uint16_t)read_reg(FIFOLevelReg),read_reg(ControlReg)&0x07,read_reg(ComIEnReg), val);//XU
			#endif
			write_reg(ComIrqReg, val);		//clearerr interrupt flag

			if (val & BIT0)
			{//timeout
				status = MI_NOTAGERR;
				#if (NFC_DEBUG)
				printf("PCD_COM Time_out\r\n");
				#endif
			}
			else
			{			
				err = read_reg(ErrorReg);

				status = MI_COM_ERR;
				if ((val & wait_for) && (val & irq_en))
				{
					if (!(val & ErrIRq))
					{//exec command correctly
						status = MI_OK;
						if (recebyte)
						{
							val = 0x7F & read_reg(FIFOLevelReg);
								if(read_reg(RxModeReg) & BIT2)
									val = val - 1;
							last_bits = read_reg(ControlReg) & 0x07;
							if (len_rest + val > FSD/*MAX_TRX_BUF_SIZE*/)
							{	//receive length exceeds buffer size
								status = MI_COM_ERR;
								#if (NFC_DEBUG)
								printf("RX_LEN > 255B\r\n");
								#endif
							}
							else
							{	
								//Prevent val-1 from becoming a negative value after spi is read incorrectly
								if (last_bits && val) 
								{
								   pi->mf_length = (val-1)*8 + last_bits;
								}
								else
								{
								   pi->mf_length = val*8;
								}
								pi->mf_length += len_rest*8;

								#if (NFC_DEBUG)
								printf(" RX:len=%02x,dat:", (uint16_t)pi->mf_length);
								#endif
								if (val == 0)
								{	
									//read length
									val = 1;
								}
								#if (NFC_DEBUG)
								printf("RX: pi_cmd=%02X,pi_len=%02X\r\n", (uint16_t)pi->mf_command, (uint16_t)pi->mf_length);
								#endif
	
								for (j = 0; j < val; j++)
								{
									pi->mf_data[len_rest + j] = read_reg(FIFODataReg);
								}

								#if (NFC_DEBUG)						
								printf("R <-: ");
								for (j = 0; j < pi->mf_length/8 + !!(pi->mf_length%8); j++)
								{
									printf("%02X ", (uint16_t)pi->mf_data[j]);
								}
								printf("l=%d", pi->mf_length/8 + !!(pi->mf_length%8));
								printf("\r\n");
								#endif
							}
						}
					}
					//NEED_MODIFY error control
					else if ((err & CollErr)/* && (!(read_reg(CollReg) & BIT5))*/)
					{//a bit-collision is detected
						status = MI_COLLERR;
						if (recebyte)
						{
							val =	0x7F & read_reg(FIFOLevelReg);
							last_bits = read_reg(ControlReg) & 0x07;
							if (len_rest + val > FSD/*MAX_TRX_BUF_SIZE*/)
							{	//received length exceeds buffer size
								#if (NFC_DEBUG)
								printf("COLL RX_LEN > 255B\r\n");
								#endif
							}
							else
							{
								//Prevent val-1 from becoming a negative value after spi is read incorrectly
								if (last_bits && val)
								{
								   pi->mf_length = (val-1)*8 + last_bits;
								}
								else
								{
								   pi->mf_length = val*8;
								}
								pi->mf_length += len_rest*8;
								#if (NFC_DEBUG)
								printf(" RX: pi_cmd=%02x,pi_len=%02x,pi_dat:", (uint16_t)pi->mf_command, (uint16_t)pi->mf_length);
								#endif
								if (val == 0)
								{
								   val = 1;
								}
								for (j = 0; j < val; j++)
								{
									pi->mf_data[len_rest + j +1] = read_reg(FIFODataReg);				
								}
								#if (NFC_DEBUG)
						        for (j = 0; j < pi->mf_length/8 + !!(pi->mf_length%8); j++)
						        {
									printf("%02X ", (uint16_t)pi->mf_data[j+1]);
						        }
								printf("\r\n");
								#endif
							}
						}
						/* The location where the collision occurred is in the first 32 bits, otherwise it is outside the 32 bits */
						if(!(read_reg(CollReg) & BIT5))
						{
							pi->mf_data[0] = (read_reg(CollReg) & CollPos);
							if (pi->mf_data[0] == 0)
							{
								pi->mf_data[0] = 32;
							}
						}
						else
						{   //This value is used to tell the upper layer that the conflict position is more than 32 bits
							pi->mf_data[0] = 33;
						}
						#if(NFC_DEBUG)
							printf("\r\n COLL_DET pos=%02x\r\n", (uint16_t)pi->mf_data[0]);
						#endif
						//Compared with the previous version, the mapping is a bit different. 
						//In order not to change the upper layer code, here is directly subtracted by one;
						pi->mf_data[0]--;

					}
					else if (err & (ProtocolErr))
					{
						#if (NFC_DEBUG)
						printf("protocol err=%02x\r\n", err);
						#endif
						status = MI_FRAMINGERR;				
					}
					else if ((err & (CrcErr | ParityErr)) && !(err &ProtocolErr) )
					{
						//EMV  parity err EMV 307.2.3.4		
						val = 0x7F & read_reg(FIFOLevelReg);
						last_bits = read_reg(ControlReg) & 0x07;
						if (len_rest + val > FSD/*MAX_TRX_BUF_SIZE*/)
						{//lenght exceeds buffer size
							status = MI_COM_ERR;
							#if (NFC_DEBUG)
							printf("RX_LEN > 255B\n");
							#endif
						}
						else
						{
							if (last_bits && val)
							{
							   pi->mf_length = (val-1)*8 + last_bits;
							}
							else
							{
							   pi->mf_length = val*8;
							}

							pi->mf_length += len_rest*8;
						}
						#if (NFC_DEBUG)
						printf("crc-parity err=%02x\n", err);
						printf("l=%d\n", pi->mf_length );
						#endif
					
						status = MI_INTEGRITY_ERR;
					}				
					else
					{
						#if (NFC_DEBUG)
						printf("unknown ErrorReg=%02x\n", (uint16_t)err);
						#endif
						status = MI_INTEGRITY_ERR;
					}
				}
				else
				{   
					status = MI_COM_ERR;
					#if (NFC_DEBUG)
					printf("MI_COM_ERR\n");
					#endif
				}
			}
			if(!(read_reg(RxModeReg) & BIT2))	//not multiple reception
			{
				break;
			}
			else if((status == MI_NOTAGERR) && (pi->mf_length != 0))
			{
				status = MI_OK;
				break;
			}
			else if((read_reg(RxModeReg) & BIT2))
			{
				write_reg(FIFOLevelReg, BIT7);	//Flush Fifo
			}
		}
 		set_bit_mask(ControlReg, BIT7);			//TStopNow =1,necessary£»
		write_reg(ComIrqReg, 0x7F);				//clear interrupt 0
		write_reg(DivIrqReg, 0x7F);				//clear interrupt 1
		clear_bit_mask(ComIEnReg, 0x7F);		//clear enable interrupt0 bit
		clear_bit_mask(DivIEnReg, 0x7F);		//clear enable interrupt1 bit
		write_reg(CommandReg, PCD_IDLE);

	}
	else
	{
		status = USER_ERROR;
		#if (NFC_DEBUG)
		printf("UNSUPPORT CMD\n");
		#endif
	}
	#if (NFC_DEBUG)
	printf("  pcd_com: sta=%d,err=%02x\n", (uint16_t)status, (uint16_t)err);
	#endif
	//GPIO_SetOutBits(HT_GPIOB, GPIO_PIN_0);
	return status;
}

void pcd_reset()
{
	#if(NFC_DEBUG)
		printf("\r\npcd_reset");
	#endif

	write_reg(CommandReg, PCD_RESETPHASE); //software reset

}


void pcd_antenna_on()
{

	write_reg(TxControlReg, read_reg(TxControlReg) | 0x03); //Tx1RFEn=1 Tx2RFEn=1

	//Single output power reduction test
	//clear_bit_mask(TxControlReg, 0x03);
	//set_bit_mask(TxControlReg, 0x02);//only 2nc route

	//clear_bit_mask(TxControlReg, 0x03);
	//set_bit_mask(TxControlReg, 0x01);//only 1st route
	Delay_ms(5);
}

void pcd_antenna_off()
{
	write_reg(TxControlReg, read_reg(TxControlReg) & (~0x03));
	Delay_ms(5);
}

void pcd_antenna_reset()
{
	pcd_antenna_off();
	pcd_antenna_on();
}

/////////////////////////////////////////////////////////////////////
// set PCD timer
// input:fwi=0~15
/////////////////////////////////////////////////////////////////////
void pcd_set_tmo(uint8_t fwi)
{
	uint32_t tmp;

	tmp = g_pcd_module_info.uc_wtxm * (1 << fwi);
	
	//ftimer = 13.56MHz/(2*2048+1) or 13.56MHz/(2*2048+2)
	write_reg(TPrescalerReg, (TP_FWT_302us) & 0xFF);
	write_reg(TModeReg, BIT7 | (((TP_FWT_302us)>>8) & 0xFF));
	
	// 302us * (1<<fwi)
	write_reg(TReloadRegL, tmp & 0xFF);
	write_reg(TReloadRegH, (tmp & 0xFF00) >> 8);
}

void pcd_delay_sfgi(uint8_t sfgi)
{
	//SFGT = (SFGT+dSFGT) = [(256 x 16/fc) x 2^SFGI] + [384/fc x 2^SFGI] 
	//dSFGT =  384 x 2^FWI / fc
    write_reg(TPrescalerReg, (TP_FWT_302us + TP_dFWT) & 0xFF);
    write_reg(TModeReg, BIT7 | (((TP_FWT_302us + TP_dFWT)>>8) & 0xFF)); 

    if (sfgi > 14 || sfgi < 1)
    {//FDTA,PCD,MIN = 6078 * 1 / fc
        sfgi = 1;
    }

    write_reg(TReloadRegL, (1 << sfgi) & 0xFF);
    write_reg(TReloadRegH, ((1 << sfgi) >> 8) & 0xFF);

    write_reg(ComIrqReg, 0x7F);	//clear int
    write_reg(ComIEnReg, BIT0);
    clear_bit_mask(TModeReg,BIT7);// clear TAuto
    set_bit_mask(ControlReg,BIT6);// set TStartNow
    
    while(!INT_PIN);// wait new INT
    //set_bit_mask(TModeReg,BIT7);// recover TAuto
    pcd_set_tmo(g_pcd_module_info.ui_fwi); //recover timeout set		
}

void pcd_lpcd_config_start(void)
{
	uint8_t WUPeriod;
	uint8_t SwingsCnt;
	vu16 tmp;

	
#if (NFC_DEBUG)
	printf("pcd_lpcd_config_start\n");
#endif
	//V1.2
	tmp = (uint16_t)((float)lpcd.inact_ms / 256 * 32.768 + 0.5);
	if(tmp>0xff) tmp = 0xff;
	WUPeriod = tmp;
	SwingsCnt = lpcd.detect_us * 27.12 / 4 / 16 + 0.5;

	pcd_reset();

	write_reg(TxControlReg, 0x8B);			//0x14, Tx2CW = 1, deliver unmodulated 13.56MHz carrier continuously, antenna on
	
	write_reg(VersionReg, PAGE4);			//0x37, page 4, disable page 4 lock(0x5E)
	write_reg(LPCDReg, 0x30 | lpcd.delta);	//0x3c, enable 32K, setting delta value
	write_reg(WUPeriodReg, WUPeriod);		//0x3d, setting sleep time, T[inactivity]=WUPeriod * 256 * Tclk_32k;
	write_reg(SwingsCntReg, 0x80 | ((lpcd.skip_times & 0x07) << 4) | (SwingsCnt & 0x0F));	//0x3e, enable LPCD, skip times, detect time
	write_reg(VersionReg, 0);			

	//Reduce false wakeups
	write_reg(VersionReg, PAGE6);			//page 6
	write_reg(CWGsN_LPCD, lpcd.u38V);		//0x38, bit7:4, conductance of the output n-driver during LPCD
	write_reg(CWGsP_LPCD, lpcd.u39V);		//0x39, bit5:0, conductance of the output p-driver during LPCD.
	write_reg(CalibReg, lpcd.u33V);			//0x33, calib mode, step=3(range: CWGsP_lpcd - 24, CWGsP_lpcd + 21)
	write_reg(ADCRefReg_LPCD, lpcd.u36V);	//0x36, default 80H 
	write_reg(VersionReg, 0);				//page 0-3
	
	//signal on pin IRQ is inverted with respect to the Status1Reg IRq bit, high level interrupt
	clear_bit_mask(ComIEnReg, BIT7);

	//write_reg(DivIEnReg, 0x20);			//0x03, enable card detection interrupt
	write_reg(DivIEnReg, 0xA0);				//0x03, enable card detection interrupt
	write_reg(CommandReg, 0x10);			//0x01, PCD soft powerdown(bit4)
}

void pcd_lpcd_end()
{
#if (NFC_DEBUG)
	printf("pcd_lpcd_end\n");
#endif
	//SYSTICK_CounterCmd(SYSTICK_COUNTER_ENABLE);
	pcd_reset();
}

uint8_t pcd_lpcd_check()
{
	if (INT_PIN && (read_reg(DivIrqReg) & BIT5)) //TagDetIrq
	{
		pcd_lpcd_end();
		write_reg(DivIrqReg, BIT5); 		//clear card detect int flag
		return TRUE;
	}
	return FALSE;
}

/**
 ****************************************************************
 * @brief pcd_set_rate() 
 *
 * set communication data rate(A/B/F)
 *
 * @param: rate = data rate
 * @param: type = protocol type
 * @return: NULL
 ****************************************************************
 */
void pcd_set_rate(uint8_t rate, uint8_t type)
{
	uint8_t val,rxwait;
	switch(rate)
	{
		case '1':
			clear_bit_mask(TxModeReg, BIT4 | BIT5 | BIT6);
			clear_bit_mask(RxModeReg, BIT4 | BIT5 | BIT6);
			write_reg(ModWidthReg, 0x26);//Miller Pulse Length

			write_reg(RxSelReg, 0x88);
			
			break;

		case '2':
			clear_bit_mask(TxModeReg, BIT4 | BIT5 | BIT6);
			set_bit_mask(TxModeReg, BIT4);
			clear_bit_mask(RxModeReg, BIT4 | BIT5 | BIT6);
			set_bit_mask(RxModeReg, BIT4);
			write_reg(ModWidthReg, 0x12);//Miller Pulse Length
		
			//Compared with 106K, rxwait needs to increase the corresponding multiple
			val = read_reg(RxSelReg);
			rxwait = ((val & 0x3F)*2);
			if (rxwait > 0x3F)
			{
				rxwait = 0x3F;
			}			
			write_reg(RxSelReg,(rxwait | (val & 0xC0)));
			break;

		case '4':			
			clear_bit_mask(TxModeReg, BIT4 | BIT5 | BIT6);
			set_bit_mask(TxModeReg, BIT5);
			clear_bit_mask(RxModeReg, BIT4 | BIT5 | BIT6);
			set_bit_mask(RxModeReg, BIT5);
			write_reg(ModWidthReg, 0x0A);//Miller Pulse Length
		
			//Compared with 106K, rxwait needs to increase the corresponding multiple
			val = read_reg(RxSelReg);
			rxwait = ((val & 0x3F)*4);
			if (rxwait > 0x3F)
			{
				rxwait = 0x3F;
			}			
			write_reg(RxSelReg,(rxwait | (val & 0xC0)));	

			if(type == 'B')
			{
				 write_reg(0x37, 0xAE);	//page5
				 write_reg(0x32, 0x6D);
				 write_reg(0x37, 0x00); //lock
			}
			break;
		case '8':			
			clear_bit_mask(TxModeReg, BIT4 | BIT5 | BIT6);
			set_bit_mask(TxModeReg, BIT4 | BIT5);
			clear_bit_mask(RxModeReg, BIT4 | BIT5 | BIT6);
			set_bit_mask(RxModeReg, BIT4 | BIT5);
			if(type == 'B')
			{
				 write_reg(0x37, 0xAE);	//page5
				 write_reg(0x32, 0x6D);
				 write_reg(0x37, 0x00); //lock
			}
			write_reg(ModWidthReg, 0x04);//Miller Pulse Length
		
			//Compared with 106K, rxwait needs to increase the corresponding multiple
			val = read_reg(RxSelReg);
			rxwait = ((val & 0x3F)*8);
			if (rxwait > 0x3F)
			{
				rxwait = 0x3F;
			}			
			write_reg(RxSelReg,(rxwait | (val & 0xC0)));	
		
			break;
		
		default:
			clear_bit_mask(TxModeReg, BIT4 | BIT5 | BIT6);
			clear_bit_mask(RxModeReg, BIT4 | BIT5 | BIT6);
			write_reg(ModWidthReg, 0x26);//Miller Pulse Length
			
			break;
	}

}

/**
 ****************************************************************
 * @brief set_bit_mask() 
 *
 * set bits fo register data
 *
 * @param: reg : register address
 * @param: mask : bit mask
 ****************************************************************
 */
void set_bit_mask(uint8_t reg, uint8_t mask)  
{
	char tmp;

	tmp = read_reg(reg);
	write_reg(reg, tmp | mask);  // set bit mask
}


/**
 ****************************************************************
 * @brief clear_bit_mask() 
 *
 * clear bits of register data
 *
 * @param: reg : register address
 * @param: mask : bit mask
 ****************************************************************
 */
void clear_bit_mask(uint8_t reg,uint8_t mask)  
{
	char tmp;

	tmp = read_reg(reg);
	write_reg(reg, tmp & ~mask);  // clear bit mask
}

/**
 ****************************************************************
 * @brief spi_write_reg() 
 *
 * write chip register
 *
 * @param:  addr : register value
 *			val	 : register data
 ****************************************************************
 */
void spi_write_reg(uint8_t addr, uint8_t val)
{
	uint8_t txBuffer[2];
	uint8_t rxBuffer[2];
	uint8_t c;
	
//The lowest bit is free, the effective data field is bit1~bit6
//The highest bit of the address is 1 for reading, and 0 for writing;
	//c = (addr <<= 1) & ~(READ_REG_CTRL);
	c = (addr << 1) & ~(READ_REG_CTRL);
	txBuffer[0] = c;
	txBuffer[1] = val;

	SPI_SET_SS_LOW(SPI_NFC_READER_PORT);
	/* SPI transmit and receive */
	SPI_TransmitReceive(SPI_NFC_READER_PORT, txBuffer, rxBuffer, 2, HAL_DELAY);
	SPI_SET_SS_HIGH(SPI_NFC_READER_PORT);
	
	//printf("write_reg:%02X = %02X\r\n", addr, val);
}

/**
 ****************************************************************
 * @brief spi_read_reg() 
 *
 * read chip register
 *
 * @param: addr : register address
 * @return: c : register value
 ****************************************************************
 */
uint8_t spi_read_reg(uint8_t addr)
{
	uint8_t txBuffer[2];
	uint8_t rxBuffer[2];
	uint8_t c, data;

//The lowest bit is free, the effective data field is bit1~bit6
//The highest bit of the address is 1 for reading, and 0 for writing;
	//c = (addr <<= 1) | READ_REG_CTRL;
	c = (addr << 1) | READ_REG_CTRL;

	txBuffer[0] = c;
	txBuffer[1] = 0x00; // dummy data

    SPI_SET_SS_LOW(SPI_NFC_READER_PORT);

	/* SPI transmit and receive data */
	SPI_TransmitReceive(SPI_NFC_READER_PORT, txBuffer, rxBuffer, 2, HAL_DELAY);
	/* Keep the data to rxBuffer */
	data = rxBuffer[1];

	SPI_SET_SS_HIGH(SPI_NFC_READER_PORT);
	
	//printf("read_reg:%02X = %02X\r\n", addr, data);
	
	return data;
}

void write_reg(uint8_t addr, uint8_t val)
{
    spi_write_reg(addr, val);	
}

uint8_t read_reg(uint8_t addr)
{
	uint8_t tmp;
    
    tmp = spi_read_reg(addr);

	return tmp;
}

