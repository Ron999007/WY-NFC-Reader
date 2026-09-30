/**
 ****************************************************************
 * @file mifare.c
 *
 * @brief  mifare protocol driver
 *
 * @author 
 *
 * 
 ****************************************************************
 */ 

/*
 * INCLUDE FILES
 ****************************************************************
 */	
#include "macro_utils.h"
#include "drv_bc45b4522.h"
#include "mifare.h"

/**
  ***************************************************** ***************
  * @brief pcd_auth_state()
  *
  * Function: Verify with the key stored in the FIFO and the key on the card
  *
  * @param: auth_mode=authentication mode, 0x60: verify A key, 0x61: verify B key
  * @param: block=The absolute block number to be verified
  * @param: psnr=Serial number first address
  * @return: status value is MI_OK: success
  *
  ***************************************************** ***************
*/
int pcd_auth_state(uint8_t auth_mode, uint8_t block, uint8_t *psnr, uint8_t *pkey)
{
	int status;
	uint8_t i;
	
	transceive_buffer  *pi;
	pi = &mf_com_data;

#if (NFC_DEBUG)
	printf("AUTH:\r\n");
#endif
	write_reg(BitFramingReg,0x00);	// // Tx last bits = 0, rx align = 0
	
	
	pcd_set_tmo(4);

	mf_com_data.mf_command = PCD_AUTHENT;
	mf_com_data.mf_length = 12;
	mf_com_data.mf_data[0] = auth_mode;
	mf_com_data.mf_data[1] = block;
	for (i = 0; i < 6; i++)
	{
		mf_com_data.mf_data[2+i] = pkey[i];
	}
	memcpy(&mf_com_data.mf_data[8], psnr, 4);

	status = pcd_com_transceive(pi);
	
	if (MI_OK == status)
	{
		if (read_reg(Status2Reg) & BIT3) //MFCrypto1On
		{
			status = MI_OK;
		}else
		{
			status = MI_AUTHERR;
		}
	}
	
	return status;

}



/**
  *************************************************** ****************
 * @brief pcd_read() 
 *
  * Function: Read a block of data (16 bytes) on the mifare_one card
 *
  * @param: addr = absolute block number to read
  * @param: preaddata = The first address of the data buffer that stores the read data
  * @return: status value / MI_OK: success
  * @retval: preaddata read data
 *
  *************************************************** ******************
 */
int pcd_read(uint8_t addr,uint8_t *preaddata)
{
    int status;
    
    transceive_buffer  *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("READ:\r\n");
#endif

	write_reg(BitFramingReg,0x00); // // Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); // TX CRC enable 
	set_bit_mask(RxModeReg, BIT7); // RX CRC enable 
	pcd_set_tmo(4);

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 2;
    mf_com_data.mf_data[0] = PICC_READ;
    mf_com_data.mf_data[1] = addr;

    status = pcd_com_transceive(pi);
    if (status == MI_OK)
    {
        if (mf_com_data.mf_length != 0x80)
        {
			status = MI_BITCOUNTERR;
		}
        else
        {
			memcpy(preaddata, &mf_com_data.mf_data[0], 16);
		}
    }
	
    return status;
}


/**
  *************************************************** ******************
 * @brief pcd_write() 
 *
  * Function: Write data to a block on the card
 *
  * @param: addr = absolute block number to be written
  * @param: pwritedata = The first address of the buffer area where the written data is stored
  * @return: status value / MI_OK: success
 *
  *************************************************** ******************
 */
int pcd_write(uint8_t addr,uint8_t *pwritedata)
{
    int status;
    
    transceive_buffer  *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("WRITE:\r\n");
#endif

    write_reg(BitFramingReg,0x00);		//Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 		//enable tx crc
	clear_bit_mask(RxModeReg, BIT7); 	//disable rx crc
	
	pcd_set_tmo(5);

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 2;
    mf_com_data.mf_data[0] = PICC_WRITE;
    mf_com_data.mf_data[1] = addr;

    status = pcd_com_transceive(pi);
    if (status != MI_NOTAGERR)
    {
        if(mf_com_data.mf_length != 4)
        {
			status=MI_BITCOUNTERR;
		}
        else
        {
           mf_com_data.mf_data[0] &= 0x0F;
           switch (mf_com_data.mf_data[0])
           {
              case 0x00:
                 status = MI_NOTAUTHERR;
                 break;
              case 0x0A:
                 status = MI_OK;
                 break;
              default:
                 status = MI_CODEERR;
                 break;
           }
        }
    }
    if (status == MI_OK)
    {	
       	pcd_set_tmo(5);

        mf_com_data.mf_command = PCD_TRANSCEIVE;
        mf_com_data.mf_length  = 16;
        memcpy(&mf_com_data.mf_data[0], pwritedata, 16);
        
        status = pcd_com_transceive(pi);
        if (status != MI_NOTAGERR)
        {
            mf_com_data.mf_data[0] &= 0x0F;
            switch(mf_com_data.mf_data[0])
            {
               case 0x00:
                  status = MI_WRITEERR;
                  break;
               case 0x0A:
                  status = MI_OK;
                  break;
               default:
                  status = MI_CODEERR;
                  break;
           }
        }
        pcd_set_tmo(4);
    }
    return status;
}

/**
  *************************************************** ******************
 * @brief pcd_write_ultralight() 
 *
  * Function: Write data to a block on the card
 *
  * @param: addr = absolute block number to be written
  * @param: pwritedata = The first address of the buffer area where the written data is stored
  * @return: status value / MI_OK: success
 *
  *************************************************** ******************
 */
int pcd_write_ultralight(uint8_t addr,uint8_t *pwritedata)
{
    int status;
    
    transceive_buffer  *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("WRITE_UL:\r\n");
#endif

    write_reg(BitFramingReg,0x00);	// Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 	// enable tx crc
	clear_bit_mask(RxModeReg, BIT7);// disable rx crc
	pcd_set_tmo(5);

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 6; //a2h ADR D0 D1 D2 D3
    mf_com_data.mf_data[0] = PICC_WRITE_ULTRALIGHT;
    mf_com_data.mf_data[1] = addr;
	memcpy(&mf_com_data.mf_data[2], pwritedata, 4);

    status = pcd_com_transceive(pi);
    
    if (status != MI_NOTAGERR)
    {
        mf_com_data.mf_data[0] &= 0x0F;
        switch(mf_com_data.mf_data[0])
        {
           case 0x00:
              status = MI_WRITEERR;
              break;
           case 0x0A:
              status = MI_OK;
              break;
           default:
              status = MI_CODEERR;
              break;
       }
    }
	pcd_set_tmo(4);

    return status;
}

/**
  *************************************************** ******************
 * @brief pcd_valueblock_operation(uint8_t mode,uint8_t addr,uint8_t *pwritedata) 
 *
  * Function: decrease(0xC0)/increase(0xC1)/restore value(0xC2)
 *
  * @param: mode = command word
  * @param: addr = address to increase/decrease value
  * @param: pwritedata = four-byte value of increase/decrease, low-order bit first
  * @return: status value / MI_OK: success
 *
  *************************************************** ******************
 */
int pcd_valueblock_operation(uint8_t mode,uint8_t addr,uint8_t *pwritedata)
{
    int status;
	   
    transceive_buffer  *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("VALUE BLOCK OP:\r\n");
#endif

    write_reg(BitFramingReg,0x00);	// // Tx last bits = 0, rx align = 0
	set_bit_mask(TxModeReg, BIT7); 
	clear_bit_mask(RxModeReg, BIT7); 
	
	pcd_set_tmo(5);

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 2;
    mf_com_data.mf_data[0] = mode;
    mf_com_data.mf_data[1] = addr;

    status = pcd_com_transceive(pi);
    if (status != MI_NOTAGERR)
    {
        if(mf_com_data.mf_length != 4)
        {
			status=MI_BITCOUNTERR;
		}
        else
        {
           mf_com_data.mf_data[0] &= 0x0F;
           switch (mf_com_data.mf_data[0])
           {
              case 0x00:
                 status = MI_NOTAUTHERR;
                 break;
              case 0x0A:
                 status = MI_OK;
                 break;
              default:
                 status = MI_CODEERR;
                 break;
           }
        }
    }
    if (status == MI_OK)
    {	
       	pcd_set_tmo(5);

        mf_com_data.mf_command = PCD_TRANSCEIVE;
        mf_com_data.mf_length  = 4;
        memcpy(&mf_com_data.mf_data[0], pwritedata, 4);
        
        status = pcd_com_transceive(pi);
        if (status != MI_NOTAGERR)
        {
            mf_com_data.mf_data[0] &= 0x0F;
            switch(mf_com_data.mf_data[0])
            {
               case 0x00:
                  status = MI_WRITEERR;
                  break;
               case 0x0A:
                  status = MI_OK;
                  break;
               default:
                  status = MI_CODEERR;
                  break;
           }
        }
		if (status == MI_NOTAGERR)
			   status = MI_OK;
    }
    return status;
}

int pcd_valueblock_transfer(uint8_t addr)
{
    int status;
	   
    transceive_buffer  *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("VALUE BLOCK TRANS:\r\n");
#endif

    write_reg(BitFramingReg,0x00);	
	set_bit_mask(TxModeReg, BIT7); 
	clear_bit_mask(RxModeReg, BIT7); 

	pcd_set_tmo(5);

	mf_com_data.mf_command = PCD_TRANSCEIVE;
	mf_com_data.mf_length  = 2;
	mf_com_data.mf_data[0] = PICC_TRANSFER;
	mf_com_data.mf_data[1] = addr;
	        
	status = pcd_com_transceive(pi);

	if (status != MI_NOTAGERR)
    {
        mf_com_data.mf_data[0] &= 0x0F;
        switch(mf_com_data.mf_data[0])
        {
           case 0x00:
              status = MI_WRITEERR;
              break;
           case 0x0A:
              status = MI_OK;
              break;
           default:
              status = MI_CODEERR;
              break;
       }
    }

	return status;

}



