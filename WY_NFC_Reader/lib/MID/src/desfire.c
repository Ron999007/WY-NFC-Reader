#include <string.h>
#include "ht32.h"
#include "desfire.h"
#include "main.h"
#include "des.h"
#include "aes.h"

#include "htk_uart.h"
#include "bc45b4522.h"

// Key to be used for DES encryption/decryption
uint8_t Key[24] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // key 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // key 2
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // key 3
} ;
uint8_t IV[16] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

uint8_t  rspData[64] = {0};
uint16_t rspDataLen = 0;
uint8_t pui8SessionKey[16];

uint8_t mfDesfire_rndA[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
uint8_t mfDesfire_en_rndA_x_rndB_P[32] = {0};

uint8_t PCB = 0x0A;
uint8_t FID;									
uint8_t AID[3];									
									

void DisplayStatus(uint8_t status)
{
	switch(status)
	{
		case 0x0c: printf("%s", "\r\n --- no changes\r\n"); break;
		case 0x0e: printf("%s", "\r\n --- out of eeprom error\r\n"); break;
		case 0x1c: printf("%s", "\r\n --- illegal command code\r\n"); break;
		case 0x1e: printf("%s", "\r\n --- integrity error\r\n"); break;
		case 0x40: printf("%s", "\r\n --- no such key\r\n"); break;
		case 0x7e: printf("%s", "\r\n --- length error\r\n"); break;
		case 0x9d: printf("%s", "\r\n --- permission denied\r\n"); break;
		case 0x9e: printf("%s", "\r\n --- parameter error\r\n"); break;
		case 0xa0: printf("%s", "\r\n --- application not found\r\n"); break;
		case 0xa1: printf("%s", "\r\n --- application integrity error\r\n"); break;
		case 0xae: printf("%s", "\r\n --- authentication error\r\n"); break;
		case 0xaf: printf("%s", "\r\n --- additional frame\r\n"); break;
		case 0xbe: printf("%s", "\r\n --- boundary error\r\n"); break;
		case 0xc1: printf("%s", "\r\n --- picc integrity error\r\n"); break;
		case 0xca: printf("%s", "\r\n --- command aborted\r\n"); break;
		case 0xcd: printf("%s", "\r\n --- picc disabled error\r\n"); break;
		case 0xce: printf("%s", "\r\n --- count error\r\n"); break;
		case 0xde: printf("%s", "\r\n --- duplicate error\r\n"); break;
		case 0xee: printf("%s", "\r\n --- eeprom error\r\n"); break;
		case 0xf0: printf("%s", "\r\n --- file not found\r\n"); break;
		case 0xf1: printf("%s", "\r\n --- file integrity error\r\n"); break;					
	}
}

void DisplayResp(char *msg, uint8_t *data, uint16_t dataLen)
{
	char  tmpBuffer[128] = {0};
	uint16_t i, j=0;
	
	printf("%s", msg);
	
	tmpBuffer[j++] = '[';
	for(i = 0; i < 2; i++)
	{
		dataToHex((uint8_t *)&tmpBuffer[j], data[i]);
		j += 2;
	}
	tmpBuffer[j++] = ']';
	tmpBuffer[j++] = '[';
	dataToHex((uint8_t *)&tmpBuffer[j], data[2]);
	j+=2;
	tmpBuffer[j++] = ']';
	for(i = 3; i < dataLen; i++)
	{
		dataToHex((uint8_t *)&tmpBuffer[j], data[i]);
		j += 2;
	}
	printf("%s", tmpBuffer);	
}

void DisplayCmd(char *msg, uint8_t *data, uint16_t dataLen)
{
	char  tmpBuffer[128] = {0};
	uint16_t i, j=0;
	
	printf("%s", msg);
	
	tmpBuffer[j++] = '[';
	for(i = 0; i < 2; i++)
	{
		dataToHex((uint8_t *)&tmpBuffer[j], data[i]);
		j += 2;
	}
	tmpBuffer[j++] = ']';
	for(i = 2; i < dataLen; i++)
	{
		dataToHex((uint8_t *)&tmpBuffer[j], data[i]);
		j += 2;
	}
	printf("%s", tmpBuffer);	
}

void DisplayArray(char *msg, uint8_t *data, uint16_t dataLen)
{
	char  tmpBuffer[128] = {0};
	uint16_t i, j=0;
	
	printf("%s", msg);
	
	for(i = 0; i < dataLen; i++)
	{
		dataToHex((uint8_t *)&tmpBuffer[j], data[i]);
		j += 2;
	}
	printf("%s", tmpBuffer);	
}

void
iso14443a_crc(uint8_t *pbtData, size_t szLen, uint8_t *pbtCrc)
{
  uint32_t wCrc = 0x6363;

  do {
    uint8_t  bt;
    bt = *pbtData++;
    bt = (bt ^ (uint8_t)(wCrc & 0x00FF));
    bt = (bt ^ (bt << 4));
    wCrc = (wCrc >> 8) ^ ((uint32_t) bt << 8) ^ ((uint32_t) bt << 3) ^ ((uint32_t) bt >> 4);
  } while (--szLen);

  *pbtCrc++ = (uint8_t)(wCrc & 0xFF);
  *pbtCrc = (uint8_t)((wCrc >> 8) & 0xFF);
}

unsigned int crc(unsigned int len, unsigned char *data) 
{ 
 unsigned int poly=0xEDB88320; 
 unsigned int crc=0xFFFFFFFF; 
 int n,b; 
 for(n=0;n<len;n++) 
 {  
  crc^=data[n]; 
  for(b=0;b<8;b++) 
   if(crc&1) 
    crc=(crc>>1)^poly; 
   else 
    crc>>=1; 
 } 
 return crc; 
} 

#define CRC32_PRESET 0xFFFFFFFF

void desfire_crc32_byte(uint32_t *crc,  uint8_t value)
{
    /* x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1 */
    const uint32_t poly = 0xEDB88320;
	short current_bit;

    *crc ^= value;
    for ( current_bit = 7; current_bit >= 0; current_bit--) {
	int bit_out = (*crc) & 0x00000001;
	*crc >>= 1;
	if (bit_out)
	    *crc ^= poly;
    }
}

uint32_t desfire_crc32( uint8_t *data,  uint8_t len)
{
	uint8_t i;
    uint32_t desfire_crc = CRC32_PRESET;
	
    for ( i = 0; i < len; i++) {
	desfire_crc32_byte(&desfire_crc, data[i]);
    }
	return desfire_crc;
}

uint8_t Desfire_AdditionalFrame(void);
uint8_t SendDesfireCommand(uint8_t *cmd, uint8_t len, char *msg1, char *msg2)
{
	uint8_t status = _ERROR_;
		
	cmd[0] = PCB;
	PCB ^= 0x01;	//toggle 
	
	DisplayCmd(msg1, cmd, len);	
	status = pcd_com_transceive_crc(cmd, len, 10, rspData, &rspDataLen);
	if(status == _SUCCESS_ )
	{
		if( rspData[2] == 0xAF)
		{
			DisplayResp(msg2, rspData, rspDataLen);
			Desfire_AdditionalFrame();
		}else
		if( rspData[2]!=0) 
		{
			printf("%s", msg2);
			DisplayStatus(rspData[2]);
			
		}else
		{
			DisplayResp(msg2, rspData, rspDataLen);
		}
	}
	else
	{
		printf("%s%s", msg2, "FAILED");
	}
	return status;
}

uint8_t Desfire_AdditionalFrame(void)
{
uint8_t mfDesfire_addFrame[] = {0x00, 0x00, 0xAF};

	
	return SendDesfireCommand(mfDesfire_addFrame, 3, "\r\n - Send Addtional Frame: ", "\r\n --- Resp. Additional Frame: ");
}

uint8_t Desfire_GetApp(void)
{
uint8_t mfDesfire_getAID[] = {0x00, 0x00, 0x6A};

return SendDesfireCommand(mfDesfire_getAID, 3, "\r\n - Send Get APP: ", "\r\n --- Resp. Get AID: ");
}

uint8_t Desfire_SelApp(uint32_t aid)
{
uint8_t mfDesfire_selAID[] = {0x00, 0x00, 0x5A, 0x00, 0x00, 0x00};

		mfDesfire_selAID[3] = aid & 0x0000ff;							
		mfDesfire_selAID[4] = (aid>>8) & 0x0000ff;
		mfDesfire_selAID[5] = (aid>>16) & 0x0000ff;

return SendDesfireCommand(mfDesfire_selAID, 6, "\r\n - Send Select APP: ", "\r\n --- Resp. Select APP : ");
}

//
// keyset : 0F, all changeable
// keyno : bit(7:6) = (1:0) AES
// keyno : bit(7:6) = (0:1) 3K3DES
// keyno : bit(7:6) = (0:0) DES/3DES
// keyno : bit(3:0)  key count
uint8_t Desfire_CreateApp(uint32_t aid, uint8_t keyset, uint8_t keyno)
{
uint8_t mfDesfire_selAID[] = {0x00, 0x00, 0x5A, 0x00, 0x00, 0x00};
uint8_t mfDesfire_createAID[] = {0x00, 0x00, 0xCA, 0x02, 0x00, 0x00, 0x0F, 0x82};	//2key-aes

		
if( SendDesfireCommand(mfDesfire_selAID, 6, "\r\n - Send Create APP: ", "\r\n --- Resp. Create APP: ") != _SUCCESS_)
			return _ERROR_;
						
		mfDesfire_createAID[3] = aid & 0x0000ff;							
		mfDesfire_createAID[4] = (aid>>8) & 0x0000ff;
		mfDesfire_createAID[5] = (aid>>16) & 0x0000ff;
		mfDesfire_createAID[6] = keyset;
		mfDesfire_createAID[7] = keyno;
return SendDesfireCommand(mfDesfire_createAID, 8, "\r\n - Send Create APP: ", "\r\n --- Resp. Create APP: ");
}

uint8_t Desfire_FormatPICC(void)
{
uint8_t mfDesfire_formatPICC[] = {0x00, 0x00, 0xFC};	

		if ( Desfire_SelApp(0) == _ERROR_) return _ERROR_;
		if ( Desfire_Auth(0, CP_DES) == _ERROR_) return _ERROR_;
		if ( Desfire_AckAuth(CP_DES) == _ERROR_) return _ERROR_;

	return SendDesfireCommand(mfDesfire_formatPICC, 3, "\r\n - Send Format PICC: ", "\r\n --- Resp. Format PICC: ");
}

uint8_t Desfire_DeleteApp(uint32_t aid)
{
uint8_t mfDesfire_deleteAID[] = {0x00, 0x00, 0xDA, 0x03, 0x00, 0x00};	
	
		if ( Desfire_SelApp(0) == _ERROR_) return _ERROR_;
		if ( Desfire_Auth(0, CP_DES) == _ERROR_) return _ERROR_;
		if ( Desfire_AckAuth(CP_DES) == _ERROR_) return _ERROR_;

		mfDesfire_deleteAID[3] = aid & 0x0000ff;							
		mfDesfire_deleteAID[4] = (aid>>8) & 0x0000ff;
		mfDesfire_deleteAID[5] = (aid>>16) & 0x0000ff;

return SendDesfireCommand(mfDesfire_deleteAID, 6, "\r\n - Send Delete APP: ", "\r\n --- Resp. Delete APP: ");
}

uint8_t Desfire_GetKeySettings(void)
{
uint8_t mfDesfire_getKeySetting[] = {0x00, 0x00, 0x45};

return SendDesfireCommand(mfDesfire_getKeySetting, 3, "\r\n - Send Get Key Settings: ", "\r\n --- Resp. Get Key Settins: ");
}

uint8_t Desfire_Auth(uint8_t keyno, uint8_t crypto)
{
uint8_t mfDesfire_auth0[] = {0x00, 0x00, 0x0A, 0x00};

uint8_t mfDesfire_ekNoRndB[16] = {0};
uint8_t mfDesfire_ekNoRndB_P[16] = {0};
uint8_t mfDesfire_rndA_x_rndB_P[32] = {0};
uint8_t lenRnd = 8;

uint8_t  OutputMessage[64] = {0};

uint16_t length, i;
uint8_t status;

	memset(IV, 0, 16);

	mfDesfire_auth0[0] = PCB;
	PCB ^= 0x01;
	if (crypto == CP_AES) 
	{
		printf("%s", "\r\n - Send Auth. KeyNo(0xAA)");
		mfDesfire_auth0[2] = 0xAA;
		lenRnd = 16;
	}else
			printf("%s", "\r\n - Send Auth. KeyNo(0x0A)");

	mfDesfire_auth0[3] = keyno;

	status = pcd_com_transceive_crc(mfDesfire_auth0, 4, 10, rspData, &rspDataLen);
	if(status == _SUCCESS_)
	{
		
		if( rspData[2]!=0 && rspData[2]!=0xAF) 
		{
			DisplayStatus(rspData[2]);
			return _ERROR_;
		}else
		{
			DisplayResp("\r\n --- Resp KeyNo: ", rspData, rspDataLen);

			for(i = 0; i < lenRnd; i++)
			{
			  mfDesfire_ekNoRndB[i] = rspData[i + 3];
			}
			DisplayArray("\r\n --- ekNo(RndB): ", mfDesfire_ekNoRndB, lenRnd);

			if( crypto == CP_DES) length = DES(OutputMessage, 'D', Key, mfDesfire_ekNoRndB, 8, IV);
			else 
			{
				struct AES_ctx ctx;
				length = 16;
				memcpy(OutputMessage, mfDesfire_ekNoRndB,16);
				
				//GPIO_ClearOutBits(HT_GPIOB, GPIO_PIN_0);
				AES_init_ctx_iv(&ctx, Key, IV);
				AES_CBC_decrypt_buffer(&ctx, OutputMessage, 16);			
				//GPIO_SetOutBits(HT_GPIOB, GPIO_PIN_0);
				
				memcpy(IV, mfDesfire_ekNoRndB, 16);
			}

	#if 1
	// Now we have RanB, so we can already save the Session Key

	// SessionKey == RndA[0..3],RndB[0..3],RndA[12..15],RndB[12..15]
	pui8SessionKey[0] = mfDesfire_rndA[0];	// RndA[0..3]
	pui8SessionKey[1] = mfDesfire_rndA[1];
	pui8SessionKey[2] = mfDesfire_rndA[2];
	pui8SessionKey[3] = mfDesfire_rndA[3];

	pui8SessionKey[4] = OutputMessage[0];		// RndB[0..3]
	pui8SessionKey[5] = OutputMessage[1];
	pui8SessionKey[6] = OutputMessage[2];
	pui8SessionKey[7] = OutputMessage[3];

	pui8SessionKey[8] =  mfDesfire_rndA[12];			// RndA[12..15]
	pui8SessionKey[9] =  mfDesfire_rndA[13];
	pui8SessionKey[10] = mfDesfire_rndA[14];
	pui8SessionKey[11] = mfDesfire_rndA[15];

	pui8SessionKey[12] = OutputMessage[12];		// RndB[12..15]
	pui8SessionKey[13] = OutputMessage[13];
	pui8SessionKey[14] = OutputMessage[14];
	pui8SessionKey[15] = OutputMessage[15];
	DisplayArray("\r\n --- Session Key: ", pui8SessionKey, 16);
	#endif
	
	
			if(length != 0)
			{
				DisplayArray("\r\n --- RndB : ", OutputMessage, length);
			}
			else
			{
				printf("%s", "\r\n --- RndB : Decrypt failed");
				return _ERROR_;
			}

			mfDesfire_ekNoRndB_P[length-1] = OutputMessage[0];
			for(i = 1; i < length; i++)
			{
				mfDesfire_ekNoRndB_P[i - 1] = OutputMessage[i];
			}
			DisplayArray( "\r\n --- RndB': ", mfDesfire_ekNoRndB_P, length);

			for(i = 0; i < (length<<1); i++)
			{
				if (i < length)
				{
					mfDesfire_rndA_x_rndB_P[i] = mfDesfire_rndA[i] ;
				}
				else
				{
					mfDesfire_rndA_x_rndB_P[i] = mfDesfire_ekNoRndB_P[i-length];
				}
			}
			DisplayArray( "\r\n --- RndA + RndB':       ", mfDesfire_rndA_x_rndB_P, (length<<1));
			
			
			if( crypto == CP_DES) length = DES(OutputMessage, 'E', Key, mfDesfire_rndA_x_rndB_P, 16, IV);
			else
			{
				struct AES_ctx ctx;

				length = 32;
				
				memcpy(OutputMessage, mfDesfire_rndA_x_rndB_P, 32);		
				
				AES_init_ctx_iv(&ctx, Key, IV);
				AES_CBC_encrypt_buffer(&ctx, OutputMessage, 32);
				memcpy(IV, &OutputMessage[16], 16);
			}
			if(length != 0)
			{
			  for(i = 0; i < length; i++)
			  {
				  mfDesfire_en_rndA_x_rndB_P[i] = OutputMessage[i];
			  }
			  DisplayArray( "\r\n --- dkNo(RndA + RndB'): ", mfDesfire_en_rndA_x_rndB_P, length);
			}
			else
			{
				printf("%s", "\r\n --- Encryp: FAILED");
				return _ERROR_;
			}
		}
	}
	else
	{
		printf("%s", "\r\n --- FAIL");
		return _ERROR_;
	}
	//memset
	return _SUCCESS_;	
}

uint8_t  Desfire_AckAuth(uint8_t crypto)
{
size_t length;
uint8_t mfDesfire_ackAuth0[48] = {0x00, 0x00, 0xAF};	//max = 3+32
uint8_t mfDesfire_ekNoRndA_P[16] = {0};
uint8_t mfDesfire_RndA_P[16] = {0};
uint8_t mfDesfire_ck_rndA[16] = {0};

uint8_t  OutputMessage[64] = {0};
uint8_t  status;
uint16_t  i;

uint8_t valFlgAuth = MFDF_AUTH_FAIL;

		if(crypto == CP_DES) length = 16;
		else length = 32;
			
		printf("%s", "\r\n - Send Ack Auth. KeyNo(0xAF)");
		for (i = 0; i < length; i++)
		{
			mfDesfire_ackAuth0[i + 3] = mfDesfire_en_rndA_x_rndB_P[i];
		}

		mfDesfire_ackAuth0[0] = PCB;
		PCB ^= 0x01;
		status = pcd_com_transceive_crc(mfDesfire_ackAuth0, 3+length, 10, rspData, &rspDataLen);
		if(status == _SUCCESS_)
		{
			if( rspData[2]!=0) 
			{
				DisplayStatus(rspData[2]);
				return _ERROR_;
			}else
			{
				DisplayResp( "\r\n --- Resp Ack KeyNo: ", rspData, rspDataLen);
				//ISO14443A_HLTA(rspData, &rspDataLen);
				
				length = (length>>1);
				for (i = 0; i < length; i++)
				{
					mfDesfire_ekNoRndA_P[i] = rspData[i + 3];
				}
				DisplayArray( "\r\n --- ek(RndA'):  ", mfDesfire_ekNoRndA_P, length);

				if( crypto == CP_DES) length = DES(OutputMessage, 'D', Key, mfDesfire_ekNoRndA_P, 8, IV);
				else
				{
					struct AES_ctx ctx;

					memcpy(OutputMessage, mfDesfire_ekNoRndA_P,16);
					AES_init_ctx_iv(&ctx, Key, IV);
					AES_CBC_decrypt_buffer(&ctx, OutputMessage, 16);	
				}
			  
				if(length != 0)
				{
					for(i = 0; i < length; i++)
					{
						mfDesfire_RndA_P[i] = OutputMessage[i];
					}
					DisplayArray( "\r\n --- RndA':      ", mfDesfire_RndA_P, length);
				}
				else
				{
					printf("%s", "\r\n --- RndA': Decrypt failed");
					return _ERROR_;
				}

				mfDesfire_ck_rndA[0] = mfDesfire_RndA_P[length-1];
				for (i = 1; i < length; i++)
				{
					mfDesfire_ck_rndA[i] = mfDesfire_RndA_P[i - 1];
				}
				DisplayArray( "\r\n --- Check RndA: ", mfDesfire_ck_rndA, length);

				for (i = 0; i < length; i++)
				{
					if (mfDesfire_ck_rndA[i] != mfDesfire_rndA[i])
					{
						valFlgAuth = MFDF_AUTH_FAIL;
						break;
					}
					valFlgAuth = MFDF_AUTH_PASS;
				}
				if (valFlgAuth == MFDF_AUTH_PASS)
				{
					printf("%s", "\r\n --- Auth KeyNo: PASSED");
					memset(IV, 0, 16);
				}
				else
				{
					printf("%s", "\r\n --- Auth KeyNo: FAILED");
					return _ERROR_;
				}
			}
		}
		else
		{
			printf("%s", "\r\n --- FAIL");
			return _ERROR_;
		}
		return _SUCCESS_;
}

uint8_t Desfire_GetFileID(void)
{
uint8_t mfDesfire_getFileID[] = {0x00, 0x00, 0x6F};


	return SendDesfireCommand(mfDesfire_getFileID, 3, "\r\n - Send Get File ID: ", "\r\n --- Resp. Get File ID: ");
}

uint8_t Desfire_GetFileSettings(uint8_t fid)
{
uint8_t mfDesfire_getFileSettings[] = {0x00, 0x00, 0xF5, 0x03};

	mfDesfire_getFileSettings[3] = fid;
	return SendDesfireCommand(mfDesfire_getFileSettings, 4, "\r\n - Send Get File Settings: ", "\r\n --- Resp. Get File Settings: ");
}

//
//  read (msb), writre, read/write, change(lsb) ==> (byte6, byte 5)
//
uint8_t Desfire_CreateSTDFile(uint8_t fid, uint8_t comset, uint32_t size, uint8_t arRead, uint8_t arWrite, uint8_t arRW, uint8_t arChange)
{
uint8_t mfDesfire_createFile[] = {0x00, 0x00, 0xCD, 0x03, 0x04, 0x05, 0x06, 0x07, 0x00, 0x00};

	mfDesfire_createFile[3] = fid;
	mfDesfire_createFile[4] = comset;		//communication setting, plain/macing/encryption

	mfDesfire_createFile[5] = (arRW<<4) | (arChange & 0x0f);
	mfDesfire_createFile[6] = (arRead<<4) | (arWrite & 0x0f);
	mfDesfire_createFile[7] = size & 0x0000ff;							
	mfDesfire_createFile[8] = (size>>8) & 0x0000ff;
	mfDesfire_createFile[9] = (size>>16) & 0x0000ff;
	
	return SendDesfireCommand(mfDesfire_createFile, 10, "\r\n - Send Create File: ", "\r\n --- Resp. Create File: ");
}

uint8_t Desfire_DeleteFile(uint8_t fid)
{
	uint8_t mfDesfire_deleteFile[] = {0x00, 0x00, 0xDF, 0x03};
	
	mfDesfire_deleteFile[3] = fid;
	return SendDesfireCommand(mfDesfire_deleteFile, 4, "\r\n - Send Delete File: ", "\r\n --- Resp. Delete File: ");
}

uint8_t Desfire_WriteSTDFile(uint8_t fid, uint32_t offset, uint32_t len, uint8_t*data, uint8_t mode)
{
uint8_t cmd_crc[64];
uint8_t mfDesfire_wrSTDFile[64] = {0x0A, 0x00, 0x3D, 0x03, 0x05, 0x00, 0x00, 0x07,0x00, 0x00};


		//if( mode == CM_ENCRYPT)
		{
			if ( Desfire_Auth(0, CP_AES) == _ERROR_) return _ERROR_;
			if ( Desfire_AckAuth(CP_AES) == _ERROR_) return _ERROR_;
		}

		mfDesfire_wrSTDFile[3] =  fid;
		mfDesfire_wrSTDFile[4] = offset & 0x0000ff;							
		mfDesfire_wrSTDFile[5] = (offset>>8) & 0x0000ff;
		mfDesfire_wrSTDFile[6] = (offset>>16) & 0x0000ff;
		mfDesfire_wrSTDFile[7] = len & 0x0000ff;							
		mfDesfire_wrSTDFile[8] = (len>>8) & 0x0000ff;
		mfDesfire_wrSTDFile[9] = (len>>16) & 0x0000ff;
		memcpy(mfDesfire_wrSTDFile+10, data, len);

		memcpy(cmd_crc, mfDesfire_wrSTDFile, len+10);
		if( mode == CM_ENCRYPT)
		{
			struct AES_ctx ctx;
			uint8_t datax[64];
			uint32_t crc32;
						
			memcpy(datax, cmd_crc+2, len+8);
			crc32 = crc(len+8, datax);		
			crc32 = desfire_crc32(datax, len+8);
			
			memcpy(cmd_crc+len+10, &crc32, 4);
			
			DisplayArray("\r\n --- CRC32: ", cmd_crc+10+len, 4);		
			DisplayArray("\r\n --- IV: ", IV, 16);
			DisplayArray("\r\n --- RAW: ", cmd_crc, 10+len+4);
			
			memcpy(datax, cmd_crc+10, len+4);				//data only
			AES_init_ctx_iv(&ctx, pui8SessionKey, IV);		
			len = AES_CBC_encrypt_buffer(&ctx, datax, len+4);	
			memcpy(cmd_crc+10, datax, len);
		}

		return SendDesfireCommand(cmd_crc, len+10, "\r\n - Send Write STD File: ","\r\n --- Resp Write STD File: ");
}

uint8_t Desfire_ReadSTDFile(uint8_t fid, uint32_t offset, uint32_t len, uint8_t mode)
{
uint8_t status;
uint8_t mfDesfire_rdSTDFile[] = {0x00, 0x00, 0xBD, 0x03, 0x04, 0x00,0x00, 0x07, 0x00, 0x00};
		
		//if( mode == CM_ENCRYPT)
		{
			if ( Desfire_Auth(0, CP_AES) == _ERROR_) return _ERROR_;
			if ( Desfire_AckAuth(CP_AES) == _ERROR_) return _ERROR_;
		}
		mfDesfire_rdSTDFile[3] =  fid;
		mfDesfire_rdSTDFile[4] = offset & 0x0000ff;							
		mfDesfire_rdSTDFile[5] = (offset>>8) & 0x0000ff;
		mfDesfire_rdSTDFile[6] = (offset>>16) & 0x0000ff;
		mfDesfire_rdSTDFile[7] = len & 0x0000ff;							
		mfDesfire_rdSTDFile[8] = (len>>8) & 0x0000ff;
		mfDesfire_rdSTDFile[9] = (len>>16) & 0x0000ff;

		if (mode == CM_ENCRYPT)
		{		
			uint8_t cmd[64], len;
			memcpy(cmd, mfDesfire_rdSTDFile+2, 8);	//skip pcb/cid
			
			memset(IV, 0, 16);
			len = Macing(cmd, 8, pui8SessionKey, IV);	
			memcpy(IV, cmd, len);
		}
									
		status = SendDesfireCommand(mfDesfire_rdSTDFile, 10, "\r\n - Sent Read STD File : ", "\r\n --- Resp Read STD File:  ");		
		
		if (mode == CM_ENCRYPT)
		{
			struct AES_ctx ctx;
			DisplayArray("\r\n --- Session Key: ", pui8SessionKey, 16);
			DisplayArray("\r\n --- IV: ", IV, 16);
			AES_init_ctx_iv(&ctx, pui8SessionKey, IV);		
			AES_CBC_decrypt_buffer(&ctx, rspData+3, rspDataLen-3);	
			
			DisplayArray("\r\n --- Decrypted data: ", rspData+3, rspDataLen-3);
		}
		return status;
}
