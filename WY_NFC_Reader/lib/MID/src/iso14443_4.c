/**
 ****************************************************************
 * @file iso1444_4.c
 *
 * @brief  interface definition of 14443-4 half-duplex communication
 *
 * @author 
 *
 * 
 ****************************************************************
 */ 
#include "macro_utils.h"
#include "drv_bc45b4522.h"
#include "iso14443_4.h"


#define DEBUG_14443_4	1 //

#if (DEBUG_14443_4)
	#define __printf(x) printf(x)
#else
	#define __printf(x) 
#endif


#define FWI_DEFAULT	4			//


pcd_info_s g_pcd_module_info; 



void pcd_default_info()			// COS_TEST
{
	memset(&g_pcd_module_info, 0, sizeof(g_pcd_module_info));
	g_pcd_module_info.ui_fsc = 32;
	g_pcd_module_info.ui_fwi = FWI_DEFAULT;
	g_pcd_module_info.ui_sfgi = 0;
	g_pcd_module_info.uc_nad_en = 0;
	g_pcd_module_info.uc_cid_en = 0;
	g_pcd_module_info.uc_wtxm = 2; //multi
		
}




int ISO14443_4_HalfDuplexExchange( struct pcd_info_s *ppcd_info, 
                                   unsigned char *psrc, 
                                   int i_tx_n, 
                                   unsigned char *pdes, 
                                  unsigned int *pi_rx_n )
{    
	
    uint8_t status;
    int            i_tx_num= 0; 
    int            i_tx_lmt= 0;
    int            i_tx_cn = 0;
    unsigned char* acu_tx_buf = mf_com_data.mf_data;
    unsigned char* acu_rx_buf = mf_com_data.mf_data;
    unsigned char* puc_tx_buf  = acu_tx_buf;
    unsigned char* puc_rx_buf  = acu_rx_buf;
    
    int            i_s_retry   = 0;
    int            i_i_retry   = 0; 
    int            i_err_retry = 0;
    
    unsigned int   ui_wtx	   = 1;

	int s_swt_limit_count;
	
    enum exe_step_t  e_ex_step;

	transceive_buffer *pi;
	pi = &mf_com_data;


    e_ex_step   = ORG_IBLOCK;
    *pi_rx_n    = 0;
    ui_wtx     = 1;
    i_s_retry   = 0;
    i_i_retry   = 0;
    i_err_retry = 0;
    
    if( ppcd_info->ui_fsc < 16 )ppcd_info->ui_fsc = 32;
    i_tx_lmt = ppcd_info->ui_fsc - 3;
	
    if( ppcd_info->uc_nad_en )i_tx_lmt--;
    if( ppcd_info->uc_cid_en )i_tx_lmt--;
    
    i_tx_num = i_tx_n;
    s_swt_limit_count=0;
    do
    {
        switch( e_ex_step )
        {
        case ORG_IBLOCK:
            puc_tx_buf  = acu_tx_buf;
            
            if( i_tx_num > i_tx_lmt )
            {
                acu_tx_buf[0] = 0x12 | ( ppcd_info->uc_pcd_pcb & ISO14443_CL_PROTOCOL_ISN );
                if( ppcd_info->uc_cid_en )acu_tx_buf[0] |= ISO14443_CL_PROTOCOL_CID;
                if( ppcd_info->uc_nad_en )acu_tx_buf[0] |= ISO14443_CL_PROTOCOL_NAD;
                puc_tx_buf++;
                
                if( ppcd_info->uc_cid_en )*puc_tx_buf++ = ppcd_info->uc_cid & 0x0F;  
                if( ppcd_info->uc_nad_en )*puc_tx_buf++ = ppcd_info->uc_nid & 0x0F;
                
                i_tx_cn    = i_tx_lmt;
                i_tx_num  -= i_tx_lmt;
            }
            else 
            {
                acu_tx_buf[0] = 0x02 | ( ppcd_info->uc_pcd_pcb & ISO14443_CL_PROTOCOL_ISN );
                if( ppcd_info->uc_cid_en )acu_tx_buf[0] |= ISO14443_CL_PROTOCOL_CID;
                if( ppcd_info->uc_nad_en )acu_tx_buf[0] |= ISO14443_CL_PROTOCOL_NAD;
                puc_tx_buf++;
                
                if( ppcd_info->uc_cid_en )*puc_tx_buf++ = ( ppcd_info->uc_cid & 0x0F );  
                if( ppcd_info->uc_nad_en )*puc_tx_buf++ = ( ppcd_info->uc_nid & 0x0F );
                
                i_tx_cn   = i_tx_num;
                i_tx_num  = 0;
            }
            memcpy( puc_tx_buf, psrc, i_tx_cn );
            puc_tx_buf += i_tx_cn;
            psrc   += i_tx_cn;
         
            ppcd_info->uc_pcd_pcb = acu_tx_buf[0];
            
            e_ex_step = ORG_TRARCV;
        break;
        case ORG_ACKBLOCK:
            puc_tx_buf  = acu_tx_buf;
            acu_tx_buf[0] = 0xA2 | ( ppcd_info->uc_pcd_pcb & 1 );
            if( ppcd_info->uc_cid_en )acu_tx_buf[0] |= ISO14443_CL_PROTOCOL_CID;
            puc_tx_buf++;
            if( ppcd_info->uc_cid_en )*puc_tx_buf++ = ( ppcd_info->uc_cid & 0x0F );  
            e_ex_step = ORG_TRARCV;
        break;
        case ORG_NACKBLOCK:
            puc_tx_buf  = acu_tx_buf;
            acu_tx_buf[0] = 0xB2 | ( ppcd_info->uc_pcd_pcb & 1 );
            if( ppcd_info->uc_cid_en )acu_tx_buf[0] |= ISO14443_CL_PROTOCOL_CID;
            puc_tx_buf++;
            if( ppcd_info->uc_cid_en )*puc_tx_buf++ = ( ppcd_info->uc_cid & 0x0F );  
            e_ex_step = ORG_TRARCV;
        break;
        case ORG_SBLOCK:
            puc_tx_buf  = acu_tx_buf;
            *puc_tx_buf++ = 0xF2;
            *puc_tx_buf++ = ui_wtx & 0x3F;  
            e_ex_step = ORG_TRARCV;    
        break;
        case ORG_TRARCV:
           
            i_err_retry++;
            

			if( ui_wtx > 59 )
			{
				ui_wtx = 1;
				e_ex_step = NON_EVENT;
				status = ISO14443_4_ERR_PROTOCOL;
				break;
			}
			
			g_pcd_module_info.uc_wtxm = ui_wtx;


			pcd_delay_sfgi(ppcd_info->ui_sfgi);
			pcd_set_tmo(ppcd_info->ui_fwi);
			pi->mf_length = puc_tx_buf - acu_tx_buf;
			memcpy(pi->mf_data, acu_tx_buf, pi->mf_length);
			pi->mf_command = PCD_TRANSCEIVE;
			status = pcd_com_transceive(pi);
			ppcd_info->uc_pcd_txr_num = pi->mf_length / 8 + !!(pi->mf_length % 8);
			memcpy(acu_rx_buf, pi->mf_data, ppcd_info->uc_pcd_txr_num);
			ppcd_info->uc_pcd_txr_lastbits = pi->mf_length % 8;
			
           
            if (status != MI_OK)
            {

                if( i_err_retry > ISO14443_PROTOCOL_RETRANSMISSION_LIMITED ||
                    i_s_retry > ISO14443_PROTOCOL_RETRANSMISSION_LIMITED )
                {
                    e_ex_step = RCV_INVBLOCK;
                }
                else
                {
                    if( ppcd_info->uc_picc_pcb & ISO14443_CL_PROTOCOL_CHAINED )
                    {
                        e_ex_step = ORG_ACKBLOCK;
                    }
                    else
                    {
                        e_ex_step = ORG_NACKBLOCK;  
                    }
                }
            }
            else
            {
                i_err_retry = 0;
                
                if (status == MI_OK)
                {   
                    puc_rx_buf = acu_rx_buf;
                    
                    if( 0x02 == ( acu_rx_buf[0] & 0xE2 ) )
                    {
                        if(( 0 == ( acu_rx_buf[0] & 0x2 ) ) || ( ppcd_info->uc_pcd_txr_num > 254 ))
                        {
                            e_ex_step = RCV_INVBLOCK;
                        }
                        else
                        {
                            i_s_retry = 0;
                            e_ex_step  = RCV_IBLOCK;
                        }
                    }
                    else if( 0xA2 == ( acu_rx_buf[0] & 0xE6 ) )
                    {
                        if( (( ISO14443_CL_PROTOCOL_CID & acu_rx_buf[0] ) && ( ppcd_info->uc_pcd_txr_num != 2 ))
							|| (( !( ISO14443_CL_PROTOCOL_CID & acu_rx_buf[0] ) ) && ( ppcd_info->uc_pcd_txr_num != 1 ))
                          )
                        {    
                            e_ex_step = RCV_INVBLOCK;
                        }
                        else
                        {
                            i_s_retry = 0;
                            e_ex_step  = RCV_RBLOCK;
                        }
                    }
                    else if( 0xC2 == ( acu_rx_buf[0] & 0xC7 ) )
                    {   
                        if( (( ISO14443_CL_PROTOCOL_CID & acu_rx_buf[0] ) && ( ppcd_info->uc_pcd_txr_num != 3 ))
							|| (( !( ISO14443_CL_PROTOCOL_CID & acu_rx_buf[0] ) ) && ( ppcd_info->uc_pcd_txr_num != 2 ))
                          )
                        {    
                            e_ex_step = RCV_INVBLOCK;
                        }
                        else
                        {
                            i_s_retry++;
                            e_ex_step = RCV_SBLOCK;
                        }
                    }
                    else 
                    {
                        e_ex_step = RCV_INVBLOCK;
                    }
                }
                else
                {
                    
                }
            }
            
 	
            ui_wtx       = 1;
			g_pcd_module_info.uc_wtxm = ui_wtx;
        break;
        case RCV_IBLOCK:
            if( i_tx_num )
            {
                e_ex_step = RCV_INVBLOCK;
            }
            else
            {
                puc_rx_buf++;
                
                
                if( ( ppcd_info->uc_pcd_pcb & ISO14443_CL_PROTOCOL_ISN ) == 
                    ( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_ISN ) )
                {   
                    ppcd_info->uc_picc_pcb = acu_rx_buf[0];
                    
                    if( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_CHAINED )
                    {
                        if( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_CID )puc_rx_buf++;
                        if( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_NAD )puc_rx_buf++;
                         
                        
                        e_ex_step = ORG_ACKBLOCK;
                    }
                    else
                    {
                        if( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_CID )puc_rx_buf++;
                        if( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_NAD )puc_rx_buf++;
                        
                        e_ex_step = NON_EVENT;   
                    }
                    
                    
                    if( ppcd_info->uc_pcd_txr_num >= ( puc_rx_buf - acu_rx_buf ) )
                    {
                        memcpy( pdes, puc_rx_buf, ( ppcd_info->uc_pcd_txr_num - ( puc_rx_buf - acu_rx_buf ) ) );
                        pdes  += ppcd_info->uc_pcd_txr_num - ( puc_rx_buf - acu_rx_buf );
                        *pi_rx_n  += ppcd_info->uc_pcd_txr_num - ( puc_rx_buf - acu_rx_buf );
                    }
                    
                    ppcd_info->uc_pcd_pcb ^= ISO14443_CL_PROTOCOL_ISN;
                }
                else 
                {
                    e_ex_step = RCV_INVBLOCK;   
                }
            }
        break;
        case RCV_RBLOCK:
            if( acu_rx_buf[0] & 0x10 )
            {
                e_ex_step = RCV_INVBLOCK;
            }
            else
            {
                
                if( ( ppcd_info->uc_pcd_pcb & ISO14443_CL_PROTOCOL_ISN ) == 
                    ( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_ISN ) )
                {
                    
                    if( ppcd_info->uc_pcd_pcb & ISO14443_CL_PROTOCOL_CHAINED )
                    {   
                        ppcd_info->uc_pcd_pcb ^= ISO14443_CL_PROTOCOL_ISN;
                        
                        i_i_retry = 0;
                        e_ex_step = ORG_IBLOCK;
                    }
                    else
                    {
                        e_ex_step = RCV_INVBLOCK; 
                    }
                }
                else 
                {
                    i_i_retry++;
                    
                    if( i_i_retry > ISO14443_PROTOCOL_RETRANSMISSION_LIMITED )
                    {
                        e_ex_step = RCV_INVBLOCK;     
                    }
                    else
                    {
                        i_tx_num += i_tx_cn;
                        psrc -= i_tx_cn;
                        e_ex_step = ORG_IBLOCK;
                    }
                }
            }
        break;
        case RCV_SBLOCK:
            if( 0xF2 != ( acu_rx_buf[0] & 0xF7 ) )
            {
                e_ex_step = RCV_INVBLOCK;
            }
            else
            {
                puc_rx_buf = acu_rx_buf + 1;
                if( acu_rx_buf[0] & ISO14443_CL_PROTOCOL_CID )puc_rx_buf++;
                if( 0 == ( *puc_rx_buf & 0x3F ) )
                {
                    e_ex_step = RCV_INVBLOCK;
                }
                else
                {
                    s_swt_limit_count++;
                    ui_wtx = ( *puc_rx_buf & 0x3F );
                    e_ex_step = ORG_SBLOCK;
                }
            }
        break;
        case RCV_INVBLOCK:

			if (MI_NOTAGERR != status)
			{
				status = ISO14443_4_ERR_PROTOCOL;
			}
			e_ex_step = NON_EVENT;
            
        break;
        default:
        break;
        }
    }while( NON_EVENT != e_ex_step );

    return status;
}



int iso14443_4_deselect(uint8_t CID)
{
    int status;
    
	transceive_buffer *pi;
    pi = &mf_com_data;

#if (NFC_DEBUG)
	printf("DESELECT:\n");
#endif
	pcd_set_tmo(4);
    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 2;
    mf_com_data.mf_data[0] = 0xca;
    mf_com_data.mf_data[1] = CID & 0x0f;
    status = pcd_com_transceive(pi);
    return status;
}



