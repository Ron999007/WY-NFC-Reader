#include "ht32.h"

typedef enum
{
    SCAN_TAG = 1,
    RST_FIELD,
    SEND_RATS,
    SEND_PPS,
    GET_APP,
    SEL_APP,
    AUTH_KEYNO,
    ACK__KEYNO,
	GET_KEY_SETTINGS,
	CREATE_APP,
	GET_FILE_ID,
	CREATE_FILE,
    WR_FILE,
    RD_FILE,
    IDLE_STATE,
}programState_t;

#define TIMEOUT_1MS            0x07
#define TIMEOUT_2MS            0x08
#define TIMEOUT_4MS            0x09
#define TIMEOUT_8MS            0x0A
#define TIMEOUT_16MS           0x0B
#define TIMEOUT_32MS           0x0C
#define TIMEOUT_64MS           0x0D
#define TIMEOUT_128MS          0x0E
#define TIMEOUT_256MS          0x0F
#define TIMEOUT_512MS          0x10
#define TIMEOUT_1SEC           0x11
#define TIMEOUT_2SEC           0x12

#define MFDF_AUTH_PASS              1
#define MFDF_AUTH_FAIL              0

#define 	CP_DES		0x00
#define 	CP_DES3K3	0x60
#define 	CP_AES		0x80

//============================================================
// 	create std file parameter
#define  	CM_PLAIN 		0x00
#define 	CM_MAC 			0x01
#define    	CM_ENCRYPT		0x03

enum DESFireAccessRights
{
    AR_KEY0  = 0x00, // Authentication with application key 0 required (master key)
    AR_KEY1  = 0x01, // Authentication with application key 1 required
    AR_KEY2  = 0x02, // ...
    AR_KEY3  = 0x03,
    AR_KEY4  = 0x04,
    AR_KEY5  = 0x05,
    AR_KEY6  = 0x06,
    AR_KEY7  = 0x07,
    AR_KEY8  = 0x08,
    AR_KEY9  = 0x09,
    AR_KEY10 = 0x0A,
    AR_KEY11 = 0x0B,
    AR_KEY12 = 0x0C,
    AR_KEY13 = 0x0D,
    AR_FREE  = 0x0E, // Always allowed even without authentication
    AR_NEVER = 0x0F  // Always forbidden even with authentication
};
//===========================================================

uint8_t Desfire_Auth(uint8_t keyno, uint8_t crypto);
uint8_t Desfire_AckAuth(uint8_t crypto);
uint8_t Desfire_GetApp(void);
uint8_t Desfire_GetKeySettings(void);
uint8_t Desfire_CreateApp(uint32_t aid, uint8_t keyset, uint8_t keyno);
uint8_t Desfire_DeleteApp(uint32_t aid);
uint8_t Desfire_SelApp(uint32_t aid);
uint8_t Desfire_GetFileID(void);
uint8_t Desfire_GetFileSettings(uint8_t fid);
uint8_t Desfire_CreateSTDFile(uint8_t fid, uint8_t comset, uint32_t size, uint8_t arRead, uint8_t arWrite, uint8_t arRW, uint8_t arChange);
uint8_t Desfire_DeleteFile(uint8_t fid);
uint8_t Desfire_ReadSTDFile(uint8_t fid, uint32_t offset, uint32_t len, uint8_t mode);
uint8_t Desfire_WriteSTDFile(uint8_t fid, uint32_t offset, uint32_t len, uint8_t*data, uint8_t mode);

uint8_t Desfire_FormatPICC(void);


void DisplayArray(char *msg, uint8_t *data, uint16_t dataLen);
void DisplayCmd(char *msg, uint8_t *data, uint16_t dataLen);
void DisplayResp(char *msg, uint8_t *data, uint16_t dataLen);


extern uint8_t Key[];
