 /**
  ****************************************************************
  * @file iso14443_4.h
  *
  * @brief 
  *
  * @author 
  *
  * 
  ****************************************************************
  */

#ifndef ISO14443_4_H
#define ISO14443_4_H

//#include "ht32.h"

#ifdef __cplusplus
extern "C" {
#endif 


typedef struct pcd_info_s
{
    unsigned char     uc_nid;
    unsigned char     uc_nad_en;
    unsigned char     uc_cid;
    unsigned char     uc_cid_en;
    unsigned int      ui_fsc;
    unsigned int      ui_fwi;
    unsigned int      ui_sfgi;
    unsigned char     uc_pcd_pcb;
    unsigned char     uc_picc_pcb;
    unsigned int      uc_pcd_txr_num;
    unsigned int      uc_pcd_txr_lastbits;
	unsigned char	  uc_wtxm;
}pcd_info_s;


extern struct pcd_info_s g_pcd_module_info;


enum exe_step_t 
    { 
        ORG_IBLOCK = 1,
        ORG_ACKBLOCK,  
        ORG_NACKBLOCK,
        ORG_SBLOCK,
        ORG_TRARCV, 
        RCV_IBLOCK,
        RCV_RBLOCK,
        RCV_SBLOCK,
        RCV_INVBLOCK,
        NON_EVENT 
    };


#define ISO14443_PROTOCOL_RETRANSMISSION_LIMITED  ( 2 )



#define ISO14443_CL_PROTOCOL_CHAINED              ( 0x10 )
#define ISO14443_CL_PROTOCOL_CID                  ( 0x8 )
#define ISO14443_CL_PROTOCOL_NAD                  ( 0x4 )
#define ISO14443_CL_PROTOCOL_ISN                  ( 0x1 )


int ISO14443_4_HalfDuplexExchange( struct pcd_info_s *ppcd_info, 
                                   unsigned char *psrc, 
                                   int i_tx_n, 
                                   unsigned char *pdes, 
                                  unsigned int *pi_rx_n );

int ISO14443_4_SRequest( struct pcd_info_s *ppcd_info, 
                         unsigned char      ucSPcb,
                         unsigned char      ucParam );
void pcd_default_info(void);

int iso14443_4_deselect(uint8_t CID);

#define ISO14443_4_ERR_PROTOCOL  ( 0x80 )	//-128

#ifdef __cplusplus
}
#endif

#endif






