/**
 ****************************************************************
 * @file iso14443a.c
 *
 * @brief  interfaces of the 14443A protocol
 *
 * @author 
 *
 * 
 ****************************************************************
 */ 
#include "macro_utils.h"
#include "drv_bc45b4522.h"
#include "iso14443_4.h"
#include "iso14443a.h"

// 14443-4
#define PICC_RATS			  0xE0 //Request for Answer To Select
#define PICC_NAK			  0xB2 // no CID

unsigned short const gausMaxFrameSizeTable[] = 
{
    16,  24,  32,  40,  48,  64,  96,  128, 256, 
    256, 256, 256, 256, 256, 256, 256, 256, 256,
};

/**
  *************************************************** ******************
 * @brief pcd_request()
 *
  * Function: cards detection
 *
  * @param: req_code[IN]: card search method
  * 				0x52 = Find all cards in the sensing area that comply with the 14443A standard
  * 				0x26 = Looking for cards that have not entered sleep state
  * @param: ptagtype[OUT]: card type code
 *                0x4400 = Mifare_UltraLight
 *                0x4400 = Mifare_One(S50_0)
 *                0x0400 = Mifare_One(S50_3)
 *                0x0200 = Mifare_One(S70_0)
  *				  0x4200 = Mifare_One(S70_3)
 *                0x0800 = Mifare_Pro
 *                0x0403 = Mifare_ProX
 *                0x4403 = Mifare_DESFire
 * 
  * @return: MI_OK is returned successfully
 * @retval:
  *************************************************** ******************
 */
int pcd_request(uint8_t req_code, uint8_t *ptagtype)
{
	int status;
	
	transceive_buffer *pi;	//for compiler v6
	pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("\r\nREQA/WUPA:\r\n");
#endif
	write_reg(BitFramingReg,0x07);	// Tx last bytes = 7

	clear_bit_mask(TxModeReg, BIT7); //disable tx crc
	clear_bit_mask(RxModeReg, BIT7); //disable rx crc
	clear_bit_mask(Status2Reg, BIT3);//clear MF crypto1 auth flag
	pcd_set_tmo(4);
	
	mf_com_data.mf_command = PCD_TRANSCEIVE;
	mf_com_data.mf_length = 1;
	mf_com_data.mf_data[0] = req_code;

	status = pcd_com_transceive(pi);

	if (!status && mf_com_data.mf_length != 0x10)
	{
		status = MI_BITCOUNTERR;
	}
	*ptagtype = mf_com_data.mf_data[0];
	*(ptagtype + 1) = mf_com_data.mf_data[1];

	return status;
}

/**
  *************************************************** ******************
 * @brief pcd_anticoll() 
 *
  * Anti-collision function
 * @param: select_code    0x93  cascaded level 1
 *                        0x95  cascaded level 2
 *                        0x97  cascaded level 3
  * @param: psnr The first address of the memory unit where the serial number (4byte) is stored
  * @return: status value MI_OK: success
  *************************************************** ******************
 */
int pcd_cascaded_anticoll(uint8_t select_code, uint8_t coll_position, uint8_t *psnr)
{
	int status;
	uint8_t i;
	uint8_t temp;
	uint8_t bits;
	uint8_t bytes;
	uint8_t snr_check;
	uint8_t snr[5];
		
	transceive_buffer *pi;
	
	pi = &mf_com_data;
	snr_check = 0;
	coll_position = 0;
	memset(snr, 0, sizeof(snr));
#if (NFC_DEBUG)
	printf("ANT:\n");
#endif
	write_reg(BitFramingReg,0x00);		//Tx last bits = 0, rx align = 0
	clear_bit_mask(TxModeReg, BIT7); 	//disable tx crc
	clear_bit_mask(RxModeReg, BIT7); 	//disable rx crc
	pcd_set_tmo(4);
	
	do 
	{
		bits = coll_position % 8;
		if (bits != 0)
		{
			bytes = coll_position / 8 + 1;
			clear_bit_mask(BitFramingReg, TxLastBits | RxAlign);
			set_bit_mask(BitFramingReg, (TxLastBits & (bits)) | (RxAlign & (bits << 4))); // tx lastbits , rx align
		}
		else
		{
			bytes = coll_position /8;
		}
		mf_com_data.mf_command = PCD_TRANSCEIVE;
		mf_com_data.mf_data[0] = select_code;
		mf_com_data.mf_data[1] = 0x20 + ((coll_position / 8) << 4) + (bits & 0x0F);
		
		for (i = 0; i < bytes; i++)
		{
			mf_com_data.mf_data[i + 2] = snr[i];
		}
		mf_com_data.mf_length = bytes + 2;

		status = pcd_com_transceive(pi);


		temp = snr[coll_position / 8];
		if (status == MI_COLLERR)
		{
			for (i = 0; (5 >= coll_position / 8) && (i < 5 - (coll_position / 8)); i++)
	        {
		         snr[i + (coll_position / 8)] = mf_com_data.mf_data[i + 1];
	        }
	        snr[(coll_position / 8)] |= temp;
			if(mf_com_data.mf_data[0] >= bits)
			{
	        	coll_position += mf_com_data.mf_data[0] - bits;
			}
			else
			{
				#if(NFC_DEBUG)
				printf("Err:coll_p  mf_data[0]=%02x < bits=%02x\n", (uint16_t)mf_com_data.mf_data[0] ,  (uint16_t)bits);
				#endif
			}


			//Preserve valid bits before conflicting bits
			snr[(coll_position / 8)] &= (0xff >> (8 - (coll_position % 8)));
			//Select the card whose conflict bit is 1 or 0
			snr[(coll_position / 8)] |=  1 << (coll_position % 8);		//Select the card with bit=1
			//snr[(coll_position / 8)] &=  ~(1 << (coll_position % 8));	//Select the card with bit=0
			coll_position++; //The conflict bit position is increased by 1
		}
		else if (status == MI_OK)
        {
            for (i=0; i < (mf_com_data.mf_length / 8) && (i <= 4); i++) //(i <= 4) prevent snr[4-i] from overflowing
            {
                 snr[4 - i] = mf_com_data.mf_data[mf_com_data.mf_length / 8 - i - 1];
            }
            snr[(coll_position / 8)] |= temp;
        }

	}while (status == MI_COLLERR);

	if (status == MI_OK)
	{
		for (i = 0; i < 4; i++)
		{
			*(psnr + i) = snr[i];
			snr_check ^= snr[i];
		}
		if (snr_check != snr[i])
		{
			status = MI_COM_ERR;
		}
	}
	
	write_reg(BitFramingReg,0x00);	// // Tx last bits = 0, rx align = 0
	
	return status;
}

/**
 ****************************************************************
 * @brief pcd_cascaded_select() 
 *
 * select card
 * @param: select_code  0x93  cascaded level 1
 *                      0x95  cascaded level 2
 *                      0x97  cascaded level 3
 * @param: psnr			The first address where stores the serial number (4byte)
 * @param: psak			The first address where stores the acknowledge
 * @return: MI_OK: success 
 *
 *			  sak:
 *            Corresponding to the specification in ISO 14443, this function
 *            is able to handle extended serial numbers. Therefore more than
 *            one select_code is possible.
 *
 *            Select codes:
 *
 *            +----+----+----+----+----+----+----+----+
 *            | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 |
 *            +-|--+-|--+-|--+-|--+----+----+----+-|--+
 *              |    |    |    |  |              | |
 *                                |              |
 *              1    0    0    1  | 001..std     | 1..bit frame anticoll
 *                                | 010..double  |
 *                                | 011..triple  |
 *
 *            SAK:
 *
 *            +----+----+----+----+----+----+----+----+
 *            | b8 | b7 | b6 | b5 | b4 | b3 | b2 | b1 |
 *            +-|--+-|--+-|--+-|--+-|--+-|--+-|--+-|--+
 *              |    |    |    |    |    |    |    |
 *                        |              |
 *                RFU     |      RFU     |      RFU
 *
 *                        1              0 .. UID complete, ATS available
 *                        0              0 .. UID complete, ATS not available
 *                        X              1 .. UID not complete
 *
 ****************************************************************
 */
int pcd_cascaded_select(uint8_t select_code, uint8_t *psnr,uint8_t *psak)
{
    uint8_t i;
    int status;
    uint8_t snr_check;
	
	transceive_buffer *pi;
    pi = &mf_com_data;
	snr_check = 0;
#if (NFC_DEBUG)
	printf("SELECT:\n");
#endif
	write_reg(BitFramingReg,0x00);	// Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 	//enable tx crc
	set_bit_mask(RxModeReg, BIT7); 	//enable rx crc
	
	pcd_set_tmo(4);

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length = 7;
    mf_com_data.mf_data[0] = select_code;
    mf_com_data.mf_data[1] = 0x70;
    for (i = 0; i < 4; i++)
    {
    	snr_check ^= *(psnr + i);
    	mf_com_data.mf_data[i + 2] = *(psnr + i);
    }
    mf_com_data.mf_data[6] = snr_check;

    status = pcd_com_transceive(pi);
    
    if (status == MI_OK)
    {		
        if (mf_com_data.mf_length != 0x8)
        {   status = MI_BITCOUNTERR;   }
        else
        {  *psak = mf_com_data.mf_data[0];  }
    }

    return status;
}

/**
 ****************************************************************
 * @brief pcd_hlta() 
 *
 * Function : card enter sleep mode
 *
 * @param:
 * @return: MI_OK: success
 *
 ****************************************************************
 */
int pcd_hlta()
{
    int status = MI_OK;
    
    transceive_buffer *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("HALT:\n");
#endif
    write_reg(BitFramingReg,0x00);		// Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 		//enable tx crc
	clear_bit_mask(RxModeReg, BIT7); 	//disable rx crc
	pcd_set_tmo(2); //according to 14443-3 1ms

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 2;
    mf_com_data.mf_data[0] = PICC_HLTA;
    mf_com_data.mf_data[1] = 0;

    status = pcd_com_transceive(pi);
    if (status)
    {
        if (status==MI_NOTAGERR || status==MI_ACCESSTIMEOUT)
        {
        	status = MI_OK;
        }
    }
    return status;
}

int pcd_rats_a(uint8_t param, uint8_t *ats, uint16_t *len)
{
    int status = MI_OK;
    
	
    transceive_buffer *pi;
	
	uint8_t *ATS;
	uint8_t ta,tb,tc;
							  
	pi = &mf_com_data;
	ta = tb = tc  = 0;

	/*initialiszed the PCB*/
    g_pcd_module_info.uc_pcd_pcb  = 0x02;
    g_pcd_module_info.uc_picc_pcb = 0x03;
    g_pcd_module_info.uc_cid = param;

	
	pcd_delay_sfgi(g_pcd_module_info.ui_sfgi);

	pcd_set_tmo(4); 								//according to 14443-4 4.8ms
	
    write_reg(BitFramingReg,0x00);					// Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 					//TX CRC enable
	set_bit_mask(RxModeReg, BIT7); 					//RX CRC enable
	
	mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 2;
    mf_com_data.mf_data[0] = PICC_RATS; 			//Start byte
    //mf_com_data.mf_data[1] = (FSDI << 4) + CID; 	//Parameter
	mf_com_data.mf_data[1] = param; 				//Parameter
	
	status = pcd_com_transceive(pi);



	if ( (status == MI_OK && (pi->mf_length % 8) != 0)
		|| (status == MI_COLLERR)
		|| (status == MI_INTEGRITY_ERR && pi->mf_length / 8 < 4)
		|| (status == MI_INTEGRITY_ERR && pi->mf_length / 8 >= 4 && (pi->mf_length % 8) != 0) 
	)
	{
		//printf("2\n");
		status = MI_NOTAGERR; 
	}
	
	if (MI_OK == status)
	{
		//ATS
		ATS = pi->mf_data;
		ats[0] = ATS[0];
		ats[1] = ATS[1];
		ats[2] = ATS[2];
			
		//printf("3\n");
		if (pi->mf_length / 8 < 1 || ATS[0] != pi->mf_length / 8)
		{//at least 1bytes, and	TL = length		
		
			//printf("4\n");
			return PROTOCOL_ERR;
		}
			
		if ( (ATS[0] < (2 + ((ATS[1]&BIT4)>>4) + ((ATS[1]&BIT5)>>5) + ((ATS[1]&BIT6)>>6))))
		{//ERR:TL length
			//return PROTOCOL_ERR;
		}
		else
		{
			if (!(ATS[1]&BIT7))
			{//T0.7 = 0
				if (ATS[1]&BIT4)
				{
					ta = 1;
				}
				if (ATS[1]&BIT5)
				{
					tb = 1;
					g_pcd_module_info.ui_fwi = (ATS[2+(uint8_t)ta] & 0xF0) >> 4;
					g_pcd_module_info.ui_sfgi = ATS[2+(uint8_t)ta] & 0x0f;
					
				}
				if (ATS[1]&BIT6)
				{
					tc = 1;
//					g_pcd_module_info.uc_cid_en = ATS[4] & 0x02; 
//					g_pcd_module_info.uc_nad_en = ATS[4] & 0x01;					
				}
				g_pcd_module_info.ui_fsc = gausMaxFrameSizeTable[ATS[1] & 0x0F];

				//FSC
				if (pi->mf_length/8 < (2 + (uint8_t)ta + (uint8_t)tb + (uint8_t)tc))
				{//wrong length
					return PROTOCOL_ERR;
				}
				pcd_set_tmo(g_pcd_module_info.ui_fwi);
			}
			else
			{
				//status = PROTOCOL_ERR;
			}
		}
	}
	*len = pi->mf_length/8;
	memcpy(ats, pi->mf_data, *len);
	

	return status;
}
 
/**
 ****************************************************************
 * @brief pcd_pps_rate() 
 *
 * Function: Send COS command to ISO14443-4 card
 *
 ****************************************************************
 */
int pcd_pps_rate(uint8_t  *ATS, uint8_t CID, uint8_t rate, uint8_t *resp, uint16_t *respLen )
{
	uint8_t DRI, DSI;
	int status = MI_OK;

#if (NFC_DEBUG)
	printf("PPS:\n");
#endif 

	DRI = 0;
	DSI = 0;

	if ((ATS[0] > 1) && (ATS[1] & BIT5))
    {//TA(1) transmited
    	if (rate == 1)
    	{
			
    	}
    	else if (rate == 2)
    	{
			#if (NFC_DEBUG)
			printf("212K\n");
			#endif
			if((ATS[2]&BIT0) && (ATS[2]&BIT4))
			{// DS=2,DR=2 supported 212kbps
				DRI = 1;
				DSI = 1;
			}
			else
			{
				#if (NFC_DEBUG)
				printf(",Unsupport\n");
				#endif
				return USER_ERROR;
			}	
    	}
		else if (rate == 3)
		{
			#if (NFC_DEBUG)
			printf("424K\n");
			#endif
			if((ATS[2]&BIT1) && (ATS[2]&BIT5))
			{// DS=4,DR=4 supported 424kbps
				DRI = 2;
				DSI = 2;
			}
			else
			{
				#if (NFC_DEBUG)
				printf(",Unsupport\n");
				#endif
				return USER_ERROR;
			}
		}
		else if (rate == 4)
		{
			#if (NFC_DEBUG)
			printf("848K\n");
			#endif
			if((ATS[2]&BIT2) && (ATS[2]&BIT6))
			{// DS=4,DR=4 supported 424kbps
				DRI = 3;
				DSI = 3;
			}
			else
			{
				#if (NFC_DEBUG)
				printf(",Unsupport\n");
				#endif
				return USER_ERROR;
			}
		}
		else
		{		
			#if (NFC_DEBUG)
			printf("USER:No Rate select\n");
			#endif
			return USER_ERROR;
		}
		write_reg(BitFramingReg,0x00);	// Tx last bits = 0, rx align = 0
		set_bit_mask(TxModeReg, BIT7); 	// TX CRC enable
		set_bit_mask(RxModeReg, BIT7); 	// RX CRC enable
	
		mf_com_data.mf_command = PCD_TRANSCEIVE;
	    mf_com_data.mf_length  = 3;
	    mf_com_data.mf_data[0] = (0x0D << 4) + CID; //Start byte
	    mf_com_data.mf_data[1] = 0x01 | BIT4; 		//PPS0 ;BIT4:PPS1 transmited
	    mf_com_data.mf_data[2] = (DSI << 2) | DRI; 	//PPS1
		status = pcd_com_transceive(&mf_com_data);
		
		if (status == MI_OK)
		{
			if (mf_com_data.mf_length == 8 && mf_com_data.mf_data[0] == ((0x0D << 4) + CID))
			{//PPS ok
				
				*respLen = mf_com_data.mf_length/8;
				memcpy(resp, mf_com_data.mf_data, *respLen);
				#if (NFC_DEBUG)	
				printf("pcd_pps_rate OK\n");
				#endif
				if (rate == 1)
				{
					//pps to 106K
					
				}
				else if (rate == 2)
				{
					//pps to 212K
					pcd_set_rate('2', 'A');// 212kbps
				}
				else if (rate == 3)
				{
					//pps to 424K
					pcd_set_rate('4', 'A');// 424kbps
				}
				else if (rate == 4)
				{
					//pps to 848K
					pcd_set_rate('8', 'A');// 848kbps
				}				
				
			}
		}

	}

	return status;
}

