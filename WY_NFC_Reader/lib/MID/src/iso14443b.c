/**
 ****************************************************************
 * @file iso14443b.c
 *
 * @brief  interfaces of the 14443A protocol
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
#include "iso14443b.h" 

//////////////////////////////////////////////////// /////////////////////
// prototype:	pcd_request_b(uint8_t req_code, uint8_t AFI, uint8_t N, uint8_t *ATQB)
// function: 	B-type card request
// IN param: 	req_code 	//request code ISO14443_3B_REQIDL 0x00 -- idle card
// 							// ISO14443_3B_REQALL 0x08 -- all cards
// 				AFI 		// application identifier, 0x00: select all
// 				N 			// The total number of time slots, the value range is 0--4.
// OUT param: 	*ATQB 		// Request response, 11 bytes
// Return: 		STATUS_SUCCESS -- success; other values -- failure.
//////////////////////////////////////////////////// /////////////////////
int pcd_request_b(uint8_t req_code, uint8_t AFI, uint8_t N, uint8_t *ATQB)
{
	int  status;
	
	transceive_buffer *pi;
    pi=&mf_com_data;

#if (NFC_DEBUG)
	printf("\r\nREQB/WUPB:");
#endif

	pcd_set_tmo(5);

	mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length = 3;
    mf_com_data.mf_data[0] = ISO14443B_ANTICOLLISION;     	// APf code, 0x05
    mf_com_data.mf_data[1] = AFI;                			// 0: all families & sub-families;
    mf_com_data.mf_data[2] = (req_code & 0x08) | (N&0x07);  // PARAM, b4=1, WUPB, b4=0, REQB
 
	status = pcd_com_transceive(pi);

    if (status!=MI_OK && status!=MI_NOTAGERR)
    {   
		status = MI_COLLERR;   
	}
    if (status == MI_OK && mf_com_data.mf_length != 96)
    {   
		status = MI_COM_ERR;   
	}
    if (status == MI_OK) 
    {	//'50'+PUPI(4bytes)+app data(4bytes)+protocol info(3bytes)+crcb(2bytes)
    	memcpy(ATQB, &mf_com_data.mf_data[0], mf_com_data.mf_length/8); 	
        pcd_set_tmo(ATQB[11]>>4); // set FWT 
        g_pcd_module_info.ui_fsc = (ATQB[10] >> 4);
//      g_pcd_module_info.uc_cid_en = ATQB[11]&0x01; 
//		g_pcd_module_info.uc_nad_en = ATQB[11]&0x02;
        g_pcd_module_info.ui_fwi = (ATQB[11]>>4);
    }
#if (NFC_DEBUG)
	printf(" sta=%d\n", status);
#endif
    return status;
}                      

//////////////////////////////////////////////////////////////////////
//SLOT-MARKER
//////////////////////////////////////////////////////////////////////
int pcd_slot_marker(uint8_t N, uint8_t *ATQB)
{
    int status;
	
	transceive_buffer *pi;
    pi = &mf_com_data;

#if (NFC_DEBUG)
	printf("SLOT:\n");
#endif
	pcd_set_tmo(5);

    if(!N || N>15)
	{
		status = MI_WRONG_PARAMETER_VALUE;	
    }
	else
    {
		mf_com_data.mf_command = PCD_TRANSCEIVE;
		mf_com_data.mf_length = 1;
		mf_com_data.mf_data[0] = 0x05 |(N << 4); // APn code

		status = pcd_com_transceive(pi);

	    if (status != MI_OK && status != MI_NOTAGERR)
	    {   
			status = MI_COLLERR;   
		}
	    if (status == MI_OK && mf_com_data.mf_length != 96)
	    {   
			status = MI_COM_ERR;   
		}
		if (status == MI_OK) 
		{	
		  memcpy(ATQB, &mf_com_data.mf_data[0], 16);
		  pcd_set_tmo(ATQB[11]>>4); // set FWT 
		  g_pcd_module_info.ui_fsc = (ATQB[10] >> 4);
//		  g_pcd_module_info.uc_cid_en = ATQB[11]&0x01; 
//		  g_pcd_module_info.uc_nad_en = ATQB[11]&0x02;
		  g_pcd_module_info.ui_fwi = (ATQB[11]>>4);
		} 	
    }
	
    return status;
}                      

//////////////////////////////////////////////////// /////////////////////
// prototype: pcd_pps_rate_b(uint8_t CID, uint8_t *ATQB, uint8_t rate)
// Function: type-B card rate negotiation attri_b
//////////////////////////////////////////////////// ////////////////////
int pcd_pps_rate_b(uint8_t CID, uint8_t *ATQB, uint8_t rate)
{
	int status;
	uint8_t dsi_dri;
	//transceive_buffer *pi;
	
    //pi = &mf_com_data;

	if (rate == 1)
	{
		dsi_dri = 0;
	}
	else if (rate == 2)
	{
		printf("212K\n");
		dsi_dri = BIT2 | BIT0;
	}
	else if (rate == 4)
	{
		printf("424K\n");
		dsi_dri = BIT3 | BIT1;
	}
	else if (rate == 8)
	{
		printf("848K\n");
		dsi_dri = BIT3 | BIT2 | BIT1 | BIT0;
	}
	else
	{
		printf("USER:No Rate select\n");
		return USER_ERROR;
	}

	status = pcd_attri_b(&ATQB[1], dsi_dri, ATQB[10]&0x0f, CID, ATQB);

	if (status == MI_OK)
	{
		printf("pps ok\n");	
		if (rate == 1)
		{
			pcd_set_rate('1', 'B');// 106kbps
		}
		else if (rate == 2)
		{
			pcd_set_rate('2', 'B');// 212kbps
		}
		else if (rate == 4)
		{
			pcd_set_rate('4', 'B');// 424kbps
		}
		else if (rate == 8)
		{
			pcd_set_rate('8', 'B');// 848kbps
		}
	}
	else
	{
		printf("pps fail\n");
	}
	
	
	return status;
}             


//////////////////////////////////////////////////// /////////////////////
//ATTRIB
// prototype: 	char pcd_attrib(uint8_t *PUPI, uint8_t pro_type, uint8_t CID, uint8_t *answer)
//
// Function : 	select PICC
// parameter: 	uint8_t *PUPI 		// 4-byte PICC identifier
// 				uint8_t dsi_dri 		// PCD<-->PICC rate selection
// 				uint8_t pro_type 	// Supported protocols, specified by the ProtocolType in the request response
// Return: 		MI_OK -- success; other values -- failure.
//////////////////////////////////////////////////// /////////////////////
int pcd_attri_b(uint8_t *PUPI, uint8_t dsi_dri, uint8_t pro_type, uint8_t CID, uint8_t *answer)
{
    int  status;
	
	transceive_buffer *pi;
    pi = &mf_com_data;
	pro_type = pro_type;

#if (NFC_DEBUG)
	printf("ATTRIB:\n");
#endif
	/*initialiszed the PCB*/
    g_pcd_module_info.uc_pcd_pcb  = 0x02;
    g_pcd_module_info.uc_picc_pcb = 0x03;
    g_pcd_module_info.uc_cid = CID;

	pcd_delay_sfgi(g_pcd_module_info.ui_sfgi);
	pcd_set_tmo(g_pcd_module_info.ui_fwi);

    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 9;
    mf_com_data.mf_data[0] = ISO14443B_ATTRIB;
    memcpy(&mf_com_data.mf_data[1], PUPI, 4);
	//mf_com_data.mf_data[1] = 0x00;
	//mf_com_data.mf_data[2] = 0x00;
	//mf_com_data.mf_data[3] = 0x00;
	//mf_com_data.mf_data[4] = 0x00;
    mf_com_data.mf_data[5] = 0x00;  // EOF/SOF required, default TR0/TR1
	//EOFTEST
	//mf_com_data.mf_data[5] = 0x04; //XU //SOF not,EOF yes
	//mf_com_data.mf_data[5] = 0x08; //XU //SOF yes,EOF no
	//mf_com_data.mf_data[5] = 0x0C; //XU //SOF no,EOF no
	
	
    set_bit_mask(TypeBReg, BIT7 | BIT6); //EOF SOF required
    //write_reg(0x1E, 0x13);	//EOF SOF required
    //write_reg(0x1E, 0xD3);	//EOF SOF not required
	
    mf_com_data.mf_data[6] = ((dsi_dri << 4) | FSDI); 	//FSDI; // Max frame 64 
    mf_com_data.mf_data[7] = 0x01; //pro_type & 0x0f;  	//ISO/IEC 14443-4 compliant?;
    mf_com_data.mf_data[8] = (CID & 0x0f);    	    	// CID ,0 - 14, If the PICC does not support CID, code value (0000)b shall be used; 
    
    status  = pcd_com_transceive(pi);
	
    if (status == MI_OK)
    {	
    	*answer = mf_com_data.mf_data[0];
    } 	

#if (NFC_DEBUG)
    printf(" sta=%d\n", status);
#endif

    return status;
} 
//////////////////////////////////////////////////////////////////////
//REQUEST B
//////////////////////////////////////////////////////////////////////
int get_idcard_num(uint8_t *pid)
{
    int status;
	
	transceive_buffer *pi;
    pi = &mf_com_data;

#if (NFC_DEBUG)
	printf("ID_NUM:\n");
#endif
	//pcd_set_tmo(5);
	
    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  =5;
    mf_com_data.mf_data[0] =0x00; //ISO14443B_ANTICOLLISION;     	       // APf code
    mf_com_data.mf_data[1] =0x36;// AFI;                // 
    mf_com_data.mf_data[2] =0x00; //((req_code<<3)&0x08) | (N&0x07);  // PARAM
	mf_com_data.mf_data[3] =0x00;
	mf_com_data.mf_data[4] =0x08;
 
    status = pcd_com_transceive(pi);

    if (status == MI_OK) 
    {	
    	memcpy(pid, &mf_com_data.mf_data[0], 10);
        //pcd_set_tmo(ATQB[11]>>4); // set FWT 
    } 
    return status;
}       

//////////////////////////////////////////////////// /////////////////////
// prototype: char pcd_halt_b(uint8_t *PUPI)
// function: suspend card
// in parameters: INT8U *pPUPI // 4-byte PICC identifier
// out parameters: -
// Return value: MI_OK -- success; other values -- failure. 
//////////////////////////////////////////////////// /////////////////////
int pcd_halt_b(uint8_t *PUPI, uint8_t *answer)
{
    int status;
	
	transceive_buffer *pi;
    pi = &mf_com_data;
	
#if (NFC_DEBUG)
	printf("HALTB:\n");
#endif
	pcd_delay_sfgi(g_pcd_module_info.ui_sfgi);
    pcd_set_tmo(g_pcd_module_info.ui_fwi);
				                               // disable, ISO/IEC3390 enable	
    mf_com_data.mf_command = PCD_TRANSCEIVE;
    mf_com_data.mf_length  = 5;
    mf_com_data.mf_data[0] = ISO14443B_HLTB;
    memcpy(&mf_com_data.mf_data[1], PUPI, 4);
    
    status = pcd_com_transceive(pi);

    if (status == MI_OK)
    {	
    	*answer = mf_com_data.mf_data[0];
    } 	
	
#if (NFC_DEBUG)
	printf(" sta=%d\n", status);
#endif

    return status;
}   
