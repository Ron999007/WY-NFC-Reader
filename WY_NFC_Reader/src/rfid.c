/**
 ****************************************************************
 * @file rfid.c
 *
 * @brief  关于各项指令测试的主要接口
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
#include "iso14443_4.h"
#include "iso14443a.h" 
#include "iso14443b.h" 
#include "mifare.h" 
#include "rfid.h"

tag_info g_tag_info;


void rfid_init()
{
	memset(&g_tag_info, 0, sizeof(g_tag_info));
	pcd_default_info();
}

char com_reqa(uint8_t cmd)
{
	char status;
	uint8_t sak;
	
	//pcd_default_info();

	//GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_0);
	status = pcd_request(cmd, g_tag_info.tag_type_bytes);
	//GPIO_SetOutBits(HT_GPIOB, GPIO_PIN_0);
	//anti-collision and select card
	if (status == MI_OK)
	{
		g_tag_info.uid_length = UID_4;
		
		status = pcd_cascaded_anticoll(PICC_ANTICOLL1, 0, &g_tag_info.serial_num[0]);
			//GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_0);
		if (status == MI_OK)
		{
			status = pcd_cascaded_select(PICC_ANTICOLL1, &g_tag_info.serial_num[0], &sak);
			//GPIO_SetOutBits(HT_GPIOB, GPIO_PIN_0);
		}
	}
	//anti-collision cascade 2
	if(status == MI_OK && (sak & BIT2))
	{
		g_tag_info.uid_length = UID_7;
		memcpy(&g_tag_info.serial_num[0], &g_tag_info.serial_num[1], 3);
		status = pcd_cascaded_anticoll(PICC_ANTICOLL2, 0, &g_tag_info.serial_num[3]);
			//GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_0);
		if(status == MI_OK)
		{
			status = pcd_cascaded_select(PICC_ANTICOLL2, &g_tag_info.serial_num[3], &sak);
			//GPIO_SetOutBits(HT_GPIOB, GPIO_PIN_0);
		}
	}
	//anti-collision cascade 3
	if(status == MI_OK && (sak & BIT2))
	{
		g_tag_info.uid_length = UID_10;
		memcpy(&g_tag_info.serial_num[3], &g_tag_info.serial_num[1], 3);
		status = pcd_cascaded_anticoll(PICC_ANTICOLL3, 0, &g_tag_info.serial_num[6]);
			//GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_0);
		if(status == MI_OK)
		{
			status = pcd_cascaded_select(PICC_ANTICOLL3, &g_tag_info.serial_num[6], &sak);
			//GPIO_SetOutBits(HT_GPIOB, GPIO_PIN_0);
		}
	}
	
	return status;
}

uint8_t com_mf1_read(uint8_t block, uint8_t keytype, uint8_t *key, uint8_t *buf, uint8_t *len)
{	
	uint8_t status;
	uint8_t is_a = keytype;
	
	if(g_tag_info.uid_length == UID_4)
	{
		status = pcd_auth_state((is_a == TRUE ? PICC_AUTHENT1A : PICC_AUTHENT1B), block, &g_tag_info.serial_num[0], key);		
	}
	else if(g_tag_info.uid_length == UID_7)
	{
		status = pcd_auth_state((is_a == TRUE ? PICC_AUTHENT1A : PICC_AUTHENT1B), block, &g_tag_info.serial_num[4], key);
	}
	else if(g_tag_info.uid_length == UID_10)
	{
		status = pcd_auth_state((is_a == TRUE ? PICC_AUTHENT1A : PICC_AUTHENT1B), block, &g_tag_info.serial_num[7], key);	
	}
	
	*len = 0;
	if (status == MI_OK)
	{
		status = pcd_read(block, buf);
		if(status == MI_OK) *len = 16;
	}

	return 	status;
}

uint8_t com_mf1_write(uint8_t block, uint8_t keytype, uint8_t *key, uint8_t *data)
{
	uint8_t status;
	uint8_t is_a = keytype;
	
	if(g_tag_info.uid_length == UID_4)
	{
		status = pcd_auth_state((is_a == TRUE ? PICC_AUTHENT1A : PICC_AUTHENT1B), block, &g_tag_info.serial_num[0], key);		
	}
	else if(g_tag_info.uid_length == UID_7)
	{
		status = pcd_auth_state((is_a == TRUE ? PICC_AUTHENT1A : PICC_AUTHENT1B), block, &g_tag_info.serial_num[4], key);
	}
	else if(g_tag_info.uid_length == UID_10)
	{
		status = pcd_auth_state((is_a == TRUE ? PICC_AUTHENT1A : PICC_AUTHENT1B), block, &g_tag_info.serial_num[7], key);	
	}
	
	if (status == MI_OK)
	{
		status = pcd_write(block, data);
	}

	return status;
}


uint8_t com_mf0_read(uint8_t block, uint8_t *buf, uint8_t *len)
{
	uint8_t status;
	
	*len = 0;
	status = pcd_read(block, buf);
	if( status == MI_OK) *len = 16;
	
	return status;
}

uint8_t com_mf0_write(uint8_t block, uint8_t *data)
{
	uint8_t status;
	
	status = pcd_write_ultralight(block, data);
	
	return status;
}

char com_reqb(uint8_t cmd)
{
	char  status;
	uint8_t  i;
	uint8_t  cnt;
	uint8_t  req_code;

	req_code = cmd;
	
	cnt = 1;		//polling count
	while(cnt--)
	{
		status = pcd_request_b(req_code, 0, 0, g_tag_info.ATQB);
		
		if(status == MI_COLLERR)	// collision, more than one card
		{
			if((status = pcd_request_b(req_code, 0, 4, g_tag_info.ATQB)) != MI_OK)
			{
			   	for (i = 1; i < 16; i++)	//2^4=16
			   	{
			    	if((status = pcd_slot_marker(i, g_tag_info.ATQB)) == MI_OK) 
					{
						break;
			    	}
			  	}
				if (status == MI_OK)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		else if (status == MI_OK)
		{
			break;
		}
	}
	if(status == MI_OK) g_tag_info.atqb_length = mf_com_data.mf_length/8;

	return status;
}
