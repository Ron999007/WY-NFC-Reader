/**
 ****************************************************************
 * @file iso14443a.h
 *
 * @brief 
 *
 * @author 
 *
 * 
 ****************************************************************
 */
#ifndef ISO14443A_H
#define ISO14443A_H
 
#include "macro_utils.h"
#include "drv_bc45b4522.h"

/**
 * DEFINES ISO14443A COMMAND
 * commands which are handled by the tag,Each tag command is written to the
 * reader IC and transfered via RF
 ****************************************************************
 */
 
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_ANTICOLL3		  0x97				 //防冲撞
#define PICC_HLTA             0x50               //休眠

/*
 * FUNCTION DECLARATIONS
 ****************************************************************
 */
 
int pcd_request(uint8_t req_code, uint8_t *ptagtype);
int pcd_cascaded_anticoll(uint8_t select_code, uint8_t coll_position, uint8_t *psnr);
int pcd_cascaded_select(uint8_t select_code, uint8_t *psnr,uint8_t *psak);
int pcd_hlta(void);
int pcd_rats_a(uint8_t param, uint8_t *ats, uint16_t *len);
int pcd_pps_rate(uint8_t *ATS, uint8_t CID, uint8_t rate, uint8_t *resp, uint16_t *respLen);
#endif

