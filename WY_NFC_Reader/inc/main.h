#ifndef __MAIN_H__
#define __MAIN_H__

#define pcd_poweron()       (PB6 = 1)   
#define pcd_poweroff()      (PB6 = 0)


typedef enum
{
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} StatusTypeDef;


#define CONFIG_14443A   		0x3A
#define CONFIG_14443B   		0x3B


#endif
