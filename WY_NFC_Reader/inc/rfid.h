/**
 ****************************************************************
 * @file rfid.h
 *
 * @brief rfid相关数据结构和函数定义
 *
 * @author 
 *
 * 
 ****************************************************************
 */
#ifndef RFID_H
#define RFID_H


//Type A
#define COM_PKT_CMD_INIT_TYPEA					0x1E
#define COM_PKT_CMD_REQA                    	0x20
#define COM_PKT_CMD_TYPEA_HALT             		0x21
#define COM_PKT_CMD_TYPEA_MF1_READ          	0x22
#define COM_PKT_CMD_TYPEA_MF1_WRITE         	0x23
#define COM_PKT_CMD_TYPEA_MF1_VALUE_BLOCK_OP	0x53
#define COM_PKT_CMD_TYPEA_MF0_READ              0x28
#define COM_PKT_CMD_TYPEA_MF0_WRITE             0x29
#define COM_PKT_CMD_TYPEA_RATS					0x2A
#define COM_PKT_CMD_EXCHANGE					0x2B
#define COM_PKT_CMD_DESELECT					0x2C
#define COM_PKT_CMD_MULTI_EXCHANGE_TEST			0x2D
#define COM_PKT_CMD_TEST_STOP					0x2E
//TYPE B
#define COM_PKT_CMD_INIT_TYPEB					0x1F
#define COM_PKT_CMD_REQB                        0x30
#define COM_PKT_CMD_TYPEB_HALT                  0x31
#define COM_PKT_CMD_TYPEB_UID					0x37

#define COM_PKT_CMD_ERR_STA_STATISTICS			0x4B



#define UID_4 4
#define UID_7 7
#define UID_10 10


#define PICC_CID                0x00        // 0~14 随意指定
#define RATE_106K				1
#define RATE_212K				2
#define RATE_424K				3
#define RATE_848K				4

typedef struct tag_info
{
	uint8_t opt_step;
	uint8_t uid_length;
	uint8_t tag_type;
	uint8_t tag_type_bytes[2];
	uint8_t serial_num[8];
	uint8_t uncoded_key[6];
	
	uint8_t atqb_length;
	uint8_t ATQB[16];
}tag_info;

extern tag_info  g_tag_info;


void rfid_init(void);
void rfid_operation(uint8_t *pcmd);
char com_reqa(uint8_t cmd);
char com_reqb(uint8_t cmd);

uint8_t com_mf0_read(uint8_t block, uint8_t *buf, uint8_t *len);
uint8_t com_mf0_write(uint8_t block, uint8_t *data);

uint8_t com_mf1_read(uint8_t block, uint8_t keytype, uint8_t *key, uint8_t *buf, uint8_t *len);
uint8_t com_mf1_write(uint8_t block, uint8_t keytype, uint8_t *key, uint8_t *data);
#endif
