/**
 ****************************************************************
 * @file mifare.h
 *
 * @brief 
 *
 * @author 
 *
 * 
 ****************************************************************
 */ 
#ifndef MIFARE_H
#define MIFARE_H

#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_RESET            0xE0               //复位


//UltraLight
#define PICC_WRITE_ULTRALIGHT 0xA2				//超轻卡写块

                                  
int pcd_auth_state(uint8_t auth_mode,uint8_t block,uint8_t *psnr, uint8_t *pkey);      
int pcd_read(uint8_t addr,uint8_t *preaddata);                       
int pcd_write(uint8_t addr,uint8_t *pwritedata);
int pcd_write_ultralight(uint8_t addr,uint8_t *pwritedata);
int pcd_valueblock_operation(uint8_t mode,uint8_t addr,uint8_t *pwritedata);
int pcd_valueblock_transfer(uint8_t addr);
#endif
