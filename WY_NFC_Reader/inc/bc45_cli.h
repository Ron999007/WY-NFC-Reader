/*
 *  bc45_cli.h
 *
 *  Created on: March 25, 2020
 *      Author: Crystal Su
 */

#ifndef BC45_CLI_H_
#define BC45_CLI_H_

#include "mid_shell.h"
#include "iso14443a.h"
#include "iso14443b.h"
#include "iso14443_4.h"

typedef enum{
	ISO14443A_TagType = 0x01u,
	ISO14443B_TagType = 0x02u,
}tISO_TagTypeDef;

typedef enum{
	DSLEEPMODE = 0x01u,
	SLEEPMODE = 0x02u,
	NORMALMODE = 0x03u
}tMCU_LowPwrTypeDef;

typedef enum{
	A_SETUP = 0x00u,
	A_REQA,
	A_WUPA,
	A_ANTICOLA,
	A_SELA,
	A_HLTA,
	A_GETUID,
	A_TRANS_CRC,
	A_TRANS_NOCRC,
	A_NOCOMMAND
}nfcA_Command_t;

typedef enum{
	B_SETUP = 0x00u,
	B_REQB,
	B_WUPB,
	B_ATTRI,
	B_HLTB,
	B_GETUID,
	B_TRANS_CRC,
	B_TRANS_NOCRC,
	B_NOCOMMAND
}nfcB_Command_t;


#define T2T_READ		0x10
#define T2T_WRITE		0x20

//extern uint8_t runningLoop;	//1:low power scan 2:engneering mode scan
//extern uint8_t typeMask;

/* General command */
int bc45_shell_command_help(shell_cmd_args * args);
int bc45_shell_command_info(shell_cmd_args * args);
int bc45_shell_command_register(shell_cmd_args * args);
int bc45_shell_command_rf(shell_cmd_args * args);
int bc45_shell_command_cdmode(shell_cmd_args * args);
int bc45_shell_command_nfcA(shell_cmd_args * args);
int bc45_shell_command_nfcB(shell_cmd_args * args);
int bc45_shell_command_mifare(shell_cmd_args *args);
int bc45_shell_command_desfire(shell_cmd_args *args);
int bc45_shell_command_t2t(shell_cmd_args * args);
int bc45_shell_command_reset(shell_cmd_args * args);
int bc45_shell_command_setup(shell_cmd_args * args);
int bc45_shell_command_polling_loop(shell_cmd_args * args);

uint8_t bc45__shellProcess(uint8_t * commandLine);
uint8_t bc45__commandPrompt(uint8_t * pBuffer, uint8_t * pBufferLen,
		                      uint8_t * prompt,  uint8_t promptLen);

uint8_t BC45_Configuration(uint8_t tagtype);
uint8_t ScanUID_ISO14443ATagType(void);
uint8_t FD_Config(void);
uint8_t ScanUID_ISO14443BTagType(void);
uint8_t ScanCertificateFlow_ISO14443ATagType(uint8_t *Running_state);
uint8_t ScanCertificateFlow_ISO14443BTagType(uint8_t *Running_state);
void ScanCertificateFlow(void);

uint8_t BC45_ConfigurationNoOfffield(uint8_t tagtype);
void UIDTypeA_BytesToChar(uint8_t *bytesUID, uint8_t *charUID, uint16_t  *outputlen);
void UID15693_BytesToChar(uint8_t *InputUID, uint8_t *OutputUID, uint8_t *outputlen, uint8_t Inv);
void UIDTypeB_BytesToChar(uint8_t *bytesUID, uint8_t *charUID, uint16_t *outputLen);

//void Display_ADC_I_Q(void);
//void BC45_RF_OnOff(uint8_t rf_field);
void byteToChar(uint8_t *inputByte, uint8_t *outputChar);

void CommandResultAlert(uint8_t status);
void Client_Init(void);
void Client_Handler(void);
#endif /* BC45_CLI_H_ */
