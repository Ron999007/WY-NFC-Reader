#include "MyApplication.h"
#include "macro_utils.h"
#include "rfid.h"
#include "bc45_cli.h"


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/**
  * @brief  Use to convert UID Type A from data (HEX) to character.
  * @param  byesUID: Pointer points to data UID (HEX).
  * @param  charUID: Pointer to data buffer for storing
  * 			     output character of data UID.
  * @param  ouputlen: Length of character of data UID.
  * @retval None
  */
void UIDTypeA_BytesToChar(uint8_t *bytesUID, uint8_t *charUID, uint16_t  *outputlen)
{
    uint8_t i;
    uint8_t j = 4;
    uint8_t temp_dataH;
    uint8_t temp_dataL;

    /*Set "UID:" to first 4 bytes of output buffer*/
    *charUID = 'U';
    *(charUID + 1) = 'I';
    *(charUID + 2) = 'D';
    *(charUID + 3) = ':';
    /*Looping following number of type A's UID*/
    for(i = 0; i < (*outputlen); i++)
    {
        temp_dataH = (*(bytesUID + i) & 0xF0) >> 4;
        if(temp_dataH <= 0x09)
        {
            temp_dataH = temp_dataH + '0';
        }
        else
        {
            temp_dataH = (temp_dataH - 0x0A) + 'A';
        }

        temp_dataL = (*(bytesUID + i) & 0x0F);
        if(temp_dataL <= 0x09)
        {
            temp_dataL = temp_dataL + '0';
        }
        else
        {
            temp_dataL = (temp_dataL - 0x0A) + 'A';
        }

        *(charUID + j) = temp_dataH;
        j++;
        *(charUID + j) = temp_dataL;
        j++;
    }
	*(charUID + j) = 0;
    *outputlen = j;
}

/**
  * @brief  Use to convert PUPI of Type B from data (HEX) to character.
  * @param  byesUID: Pointer points to data PUPI (HEX).
  * @param  charUID: Pointer to data buffer for storing
  * 			  output character of data PUPI.
  * @param	outputLen: Length of character of data PUPI.
  * @retval None
  */
void UIDTypeB_BytesToChar(uint8_t *bytesUID, uint8_t *charUID, uint16_t *outputLen)
{
    uint8_t i;
    uint8_t j = 5;
    uint8_t temp_dataH;
    uint8_t temp_dataL;

    /*Set "PUPI:" to first 5 bytes of output buffer*/
    *charUID = 'P';
    *(charUID + 1) = 'U';
    *(charUID + 2) = 'P';
    *(charUID + 3) = 'I';
    *(charUID + 4) = ':';
    /*Looping following number of type B's PUPI*/
    for(i = 0; i < 4; i++)
    {
        temp_dataH = (*(bytesUID + i) & 0xF0) >> 4;
        if(temp_dataH <= 0x09)
        {
            temp_dataH = temp_dataH + '0';
        }
        else
        {
            temp_dataH = (temp_dataH - 0x0A) + 'A';
        }

        temp_dataL = (*(bytesUID + i) & 0x0F);
        if(temp_dataL <= 0x09)
        {
            temp_dataL = temp_dataL + '0';
        }
        else
        {
            temp_dataL = (temp_dataL - 0x0A) + 'A';
        }

        *(charUID + j) = temp_dataH;
        j++;
        *(charUID + j) = temp_dataL;
        j++;
    }
    *(charUID + j) = '\r';
    j++;
    *(charUID + j) = '\n';
	j++;
	
	*(charUID + j) = 0;
    *outputLen = j;
}

/*
 * @brief Configure BC45 tag type communication
 * @param tagtype: Tag type definition
 *                 - CONFIG_14443A
 *                 - CONFIG_14443B
 * @retval status
 * */
uint8_t BC45_Configuration(uint8_t tagtype)
{	
	uint8_t Resp = USER_ERROR;
	
	
	rfid_init();
	pcd_reset();
	
	pcd_antenna_reset();
	
    /*Configure tag type communication*/
    switch(tagtype)
    {
    case CONFIG_14443A:
		Resp = pcd_config('A');
	
        if(Resp != _SUCCESS_)
        {
            printf("\r\nISO14443A Configuration failed");
        }
        else
        {
            //printf("\r\nISO14443A Configuration success");
        }
        break;
    case CONFIG_14443B:
		Resp = pcd_config('B');
        if(Resp != _SUCCESS_)
        {
            printf("\r\nISO14443B Configuration failed");
        }
        else
        {
            //printf("\r\nISO14443B Configuration success");
        }
        break;
    default:
        DBG_PRINT("\r\nBC45 Configuration failed");
        break;
    }
	
	return Resp;
}

/*
 * @brief Configure BC45 tag type communication
 * @param tagtype: Tag type definition
 *                 - CONFIG_14443A
 *                 - CONFIG_14443B
 * @retval status
 * */
uint8_t BC45_ConfigurationNoOfffield(uint8_t tagtype)
{	
	uint8_t Resp = USER_ERROR;

	rfid_init();
	//pcd_reset();
	

    /*Configure tag type communication*/
    switch(tagtype)
    {
    case CONFIG_14443A:
		Resp = pcd_config('A');	
        break;
    case CONFIG_14443B:
		Resp = pcd_config('B');
        break;
    default:
        DBG_PRINT("\r\nBC45 Configuration failed");
        break;
    }
	return Resp;
}

/*
 * @brief Scan UID of an ISO14443A tag type
 * @param	None
 * @retval 	Response from scanning
 * 			- _SUCCESS_
 * 			- NOT 0 (ERROR CODE)
 * */
char Buffer[64];
uint16_t bufLen;
uint8_t ScanUID_ISO14443ATagType(void)
{
    uint8_t Resp;

	pcd_antenna_on();

	Resp = com_reqa(PICC_REQALL);
    /*Check Response from type A transmitted command*/
	switch(Resp)
	{
    case MI_OK:
        /*Convert UID type A from HEX to CHAR*/
		bufLen = g_tag_info.uid_length;
        UIDTypeA_BytesToChar(g_tag_info.serial_num, (uint8_t*)Buffer, &bufLen);
		printf("%s\r\n", Buffer);
        //Ron Modify Start
		//CommandResultAlert(Resp);
        Delay_ms(500);
        //Ron Modify End
        break;
	default:
        DBG_PRINT("No Tag-A\r\n");
        break;
    }
    return Resp;
}

uint8_t FD_Config(void)
{
  uint8_t Resp;

	Resp = com_reqa(PICC_REQALL);
    /*Check Response from type A transmitted command*/
	switch(Resp)
	{
        case MI_OK:
//			  Ttxbuffer[0] = 0x01;
//		    Ttxbuffer[1] = 0x00;
//		    Ttxbuffer[2] = 0x1F;
//		    Ttxbuffer[3] = 0x84;//Reset FD Config
//        Resp = pcd_write_ultralight(0xE8, &Ttxbuffer[0]); 
//		    
//		    Ttxbuffer[0] = 0x69;
////		    Ttxbuffer[0] = 0x7D;
//		    Ttxbuffer[1] = 0x01;
//		    Ttxbuffer[2] = 0x1F;
//		    Ttxbuffer[3] = 0x84;//Setting FD Config
//		    Resp = pcd_write_ultralight(0xE8, &Ttxbuffer[0]); 
		
            break;
        default:
            DBG_PRINT("No Tag-A\r\n");
		    //Task_Buzzer_SetMode(BUZZER_MODE_BEEP_DOUBLE);
            break;
    }
    return Resp;	
}

/*
 * @brief Scan UID of an ISO14443B tag type
 * @retval Response from scanning
 *         - _SUCCESS_
 *         - NO_RESPONSE
 *         - ASIC_EXE_TIMEOUT
 *         - ERROR
 * */
uint8_t ScanUID_ISO14443BTagType(void)
{
    uint8_t Resp;

	pcd_antenna_on();

	Resp = com_reqb(ISO14443B_WUPB);//search b card
    /*Check Response type B : Wake up command*/
	switch(Resp)
	{
    case _SUCCESS_:
        /*Convert UID type B from HEX to CHAR*/
		bufLen = g_tag_info.atqb_length - 3;
        UIDTypeB_BytesToChar(&g_tag_info.ATQB[1], (uint8_t*)Buffer, &bufLen);
		printf("%s", Buffer);
		//CommandResultAlert(Resp);
        break;
    default:
        DBG_PRINT("No Tag-B\r\n");
        break;
	}

	return Resp;
}
