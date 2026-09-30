/*************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * @brief    A project template for M480 MCU.
 *
 * @copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "MyApplication.h"
#include "bc45_cli.h"
#include "main.h"

#define PLL_CLOCK           192000000
//#define PLL_CLOCK           160000000


#define HIRC    0

void SYS_Init(void)
{
    uint32_t volatile i;
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init System Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Unlock protected registers */
    SYS_UnlockReg();
#if HIRC
    /* Enable Internal High-Speed RC oscillator (HIRC) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Wait for HIRC clock to stabilize */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Switch HCLK clock source to HIRC temporarily before PLL configuration */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable PLL and set it to 160 MHz using HIRC as the clock source */
    CLK_EnablePLL(CLK_PLLCTL_PLLSRC_HIRC, PLL_CLOCK);

    /* Wait for PLL clock to stabilize */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* Switch HCLK clock source to PLL to run at 192 MHz */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));
    
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Update System Core Clock global variable for BSP functions (e.g., SysTick) */
    SystemCoreClockUpdate();    

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART0 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable UART0 clock */
//    CLK_EnableModuleClock(UART0_MODULE);

//    /* Select UART0 clock source from HXT */
//    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));
//    
//    /* Set GPB12&13 multi-function pins for UART0 RXD and TXD */
//    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
//    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART1 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
//    /* Enable UART1 module clock */ 
//    CLK_EnableModuleClock(UART1_MODULE);
//    
//    /* Select UART1 module clock source as HIRC and UART1 module clock divider as 1 */ 
//    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HIRC, CLK_CLKDIV0_UART1(1));
//    
//    /* Set GPB2&3 multi-function pins for UART1 RXD and TXD */
//    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk);
//    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB3MFP_UART1_TXD | SYS_GPB_MFPL_PB2MFP_UART1_RXD);
    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART2 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
//    /* Enable UART2 module clock */ 
//    CLK_EnableModuleClock(UART2_MODULE);
//    
//    /* Select UART2 module clock source as HIRC and UART2 module clock divider as 1 */
//    CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HIRC, CLK_CLKDIV4_UART2(1));
//    
//    /* Set GPB0&1 multi-function pins for UART2 RXD and TXD */
//    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB0MFP_Msk);
//    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB1MFP_UART2_TXD | SYS_GPB_MFPL_PB0MFP_UART2_RXD);
#if 1   
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART4 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable UART4 module clock */ 
    CLK_EnableModuleClock(UART4_MODULE);
    
    /* Select UART4 module clock source as HIRC and UART4 module clock divider as 1 */ 
    CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HIRC, CLK_CLKDIV4_UART4(1));
    
    /* Set GPC4&5 multi-function pins for UART4 RXD and TXD */
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC4MFP_Msk | SYS_GPC_MFPL_PC5MFP_Msk);
    SYS->GPC_MFPL |= (SYS_GPC_MFPL_PC4MFP_UART4_RXD | SYS_GPC_MFPL_PC5MFP_UART4_TXD);
#endif    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART5 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable UART5 module clock */ 
    CLK_EnableModuleClock(UART5_MODULE);
    
    /* Select UART5 module clock source as HIRC and UART5 module clock divider as 1 */ 
    CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HIRC, CLK_CLKDIV4_UART5(1));
    
    /* Set GPA4&5 multi-function pins for UART5 RXD and TXD */
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA4MFP_UART5_RXD | SYS_GPA_MFPL_PA5MFP_UART5_TXD);
   
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init Timer0 Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable TMR0 module clock */
    CLK_EnableModuleClock(TMR0_MODULE);
    
    /* Select TMR0 clock source */
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HIRC, 0);
#if 1
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I2C0 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable I2C0 peripheral clock */
    CLK_EnableModuleClock(I2C0_MODULE);
    
    /* Set I2C0 multi-function pins */
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk)) |
                    (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);
                    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I2C1 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable I2C1 peripheral clock */
    CLK_EnableModuleClock(I2C1_MODULE);
    
    /* Set I2C1 multi-function pins */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB11MFP_Msk)) |
                    (SYS_GPB_MFPH_PB10MFP_I2C1_SDA | SYS_GPB_MFPH_PB11MFP_I2C1_SCL);
#endif                   
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI0 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable SPI0 module clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    
    /* Select PCLK1 as the clock source for SPI0 */
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);

    /* Configure PA.0 ~ PA.3 as SPI0 multi-function pins */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA0MFP_Msk | 
                                       SYS_GPA_MFPL_PA1MFP_Msk | 
                                       SYS_GPA_MFPL_PA2MFP_Msk | 
                                       SYS_GPA_MFPL_PA3MFP_Msk)) | 
                    (SYS_GPA_MFPL_PA0MFP_SPI0_MOSI | 
                     SYS_GPA_MFPL_PA1MFP_SPI0_MISO | 
                     SYS_GPA_MFPL_PA2MFP_SPI0_CLK  | 
                     SYS_GPA_MFPL_PA3MFP_SPI0_SS);

    /* Enable high slew rate on PA.0 ~ PA.3 to improve high-speed signal integrity */
    GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3, GPIO_SLEWCTL_HIGH);
    
    /* Enable SPI2 clock pin (PA2) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
#if 1    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI2 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable SPI2 module peripheral clock */
    CLK_EnableModuleClock(SPI2_MODULE);
    
    /* Select PCLK0 as the clock source of SPI2 */
    CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_HIRC, MODULE_NoMsk);    
    
    /* Setup SPI2 multi-function pins */
    SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA8MFP_Msk | SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk | SYS_GPA_MFPH_PA11MFP_Msk)) | 
                    (SYS_GPA_MFPH_PA8MFP_SPI2_MOSI | SYS_GPA_MFPH_PA9MFP_SPI2_MISO | SYS_GPA_MFPH_PA10MFP_SPI2_CLK | SYS_GPA_MFPH_PA11MFP_SPI2_SS);

    /* Enable SPI2 clock pin (PA9) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN9_Msk;

    /* Enable SPI0 I/O high slew rate */
    GPIO_SetSlewCtl(PA, (BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_HIGH);
#endif
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init EPWM1 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable EPWM1 module clock */
    CLK_EnableModuleClock(EPWM1_MODULE);
    
    /* EPWM clock frequency is set double to PCLK: select EPWM module clock source as PLL */
    CLK_SetModuleClock(EPWM1_MODULE, CLK_CLKSEL2_EPWM1SEL_PLL, (uint32_t)NULL);
    
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB0MFP_EPWM1_CH5);

#else
    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~24 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);
    
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);
    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init Timer0 Clock                                                                                       */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable TMR0 module clock */
    CLK_EnableModuleClock(TMR0_MODULE);
    
    /* Select TMR0 clock source */
    CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART0 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable UART0 clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART0 clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    
    /* Set GPB12&13 multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART1 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
//    /* Enable UART1 module clock */ 
//    CLK_EnableModuleClock(UART1_MODULE);
//    
//    /* Select UART1 module clock source as HXT and UART1 module clock divider as 1 */ 
//    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, CLK_CLKDIV0_UART1(1));
//    
//    /* Set GPB2&3 multi-function pins for UART1 RXD and TXD */
//    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk);
//    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB3MFP_UART1_TXD | SYS_GPB_MFPL_PB2MFP_UART1_RXD);

    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk | SYS_GPB_MFPL_PB2MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB3MFP_GPIO | SYS_GPB_MFPL_PB2MFP_GPIO);
    GPIO_SetMode(PB, (BIT2 | BIT3), GPIO_MODE_OUTPUT);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART3 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable UART3 module clock */ 
    CLK_EnableModuleClock(UART3_MODULE);
    
    /* Select UART3 module clock source as HXT and UART3 module clock divider as 1 */ 
    CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, CLK_CLKDIV4_UART3(1));
    
    /* Set GPB14&15 multi-function pins for UART3 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB14MFP_Msk | SYS_GPB_MFPH_PB15MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB14MFP_UART3_RXD | SYS_GPB_MFPH_PB15MFP_UART3_TXD);
    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART5 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable UART5 module clock */ 
    CLK_EnableModuleClock(UART5_MODULE);
    
    /* Select UART5 module clock source as HIRC and UART5 module clock divider as 1 */ 
    CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HXT, CLK_CLKDIV4_UART5(1));
    
    /* Set GPA4&5 multi-function pins for UART5 RXD and TXD */
    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk);
    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA4MFP_UART5_RXD | SYS_GPA_MFPL_PA5MFP_UART5_TXD);
    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I2C0 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable I2C0 peripheral clock */
    CLK_EnableModuleClock(I2C0_MODULE);
    
    /* Set I2C0 multi-function pins */
    SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk)) |
                    (SYS_GPB_MFPL_PB4MFP_I2C0_SDA | SYS_GPB_MFPL_PB5MFP_I2C0_SCL);
                    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init I2C1 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable I2C1 peripheral clock */
    CLK_EnableModuleClock(I2C1_MODULE);
    
    /* Set I2C1 multi-function pins */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB11MFP_Msk)) |
                    (SYS_GPB_MFPH_PB10MFP_I2C1_SDA | SYS_GPB_MFPH_PB11MFP_I2C1_SCL);

    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI0 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable SPI0 module clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    
    /* Select PCLK1 as the clock source for SPI0 */
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);

    /* Configure PA.0 ~ PA.3 as SPI0 multi-function pins */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA0MFP_Msk | 
                                       SYS_GPA_MFPL_PA1MFP_Msk | 
                                       SYS_GPA_MFPL_PA2MFP_Msk | 
                                       SYS_GPA_MFPL_PA3MFP_Msk)) | 
                    (SYS_GPA_MFPL_PA0MFP_SPI0_MOSI | 
                     SYS_GPA_MFPL_PA1MFP_SPI0_MISO | 
                     SYS_GPA_MFPL_PA2MFP_SPI0_CLK  | 
                     SYS_GPA_MFPL_PA3MFP_SPI0_SS);

    /* Enable high slew rate on PA.0 ~ PA.3 to improve high-speed signal integrity */
    GPIO_SetSlewCtl(PA, BIT0 | BIT1 | BIT2 | BIT3, GPIO_SLEWCTL_HIGH);
    
    /* Enable SPI0 clock pin (PA2) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk; 

#if 1    
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI2 Clock                                                                                         */
    /*---------------------------------------------------------------------------------------------------------*/ 
    /* Enable SPI2 module peripheral clock */
    CLK_EnableModuleClock(SPI2_MODULE);
    
    /* Select PCLK1 as the clock source of SPI2 */
    CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_PCLK1, MODULE_NoMsk);    
    
    /* Setup SPI2 multi-function pins */
    SYS->GPA_MFPH = (SYS->GPA_MFPH & ~(SYS_GPA_MFPH_PA8MFP_Msk | SYS_GPA_MFPH_PA9MFP_Msk | SYS_GPA_MFPH_PA10MFP_Msk | SYS_GPA_MFPH_PA11MFP_Msk)) | 
                    (SYS_GPA_MFPH_PA8MFP_SPI2_MOSI | SYS_GPA_MFPH_PA9MFP_SPI2_MISO | SYS_GPA_MFPH_PA10MFP_SPI2_CLK | SYS_GPA_MFPH_PA11MFP_SPI2_SS);

    /* Enable SPI2 clock pin (PA9) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN9_Msk;

    /* Enable SPI2 I/O high slew rate */
    GPIO_SetSlewCtl(PA, (BIT8 | BIT9 | BIT10 | BIT11), GPIO_SLEWCTL_HIGH);
#endif
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init EPWM1 Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Enable EPWM1 module clock */
    CLK_EnableModuleClock(EPWM1_MODULE);
    
    /* EPWM clock frequency is set double to PCLK: select EPWM module clock source as PLL */
    CLK_SetModuleClock(EPWM1_MODULE, CLK_CLKSEL2_EPWM1SEL_PLL, (uint32_t)NULL);
    
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB0MFP_EPWM1_CH5);
#endif
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init USBD Clock                                                                                        */
    /*---------------------------------------------------------------------------------------------------------*/
    /* select HSUSBD */
    SYS->USBPHY &= ~SYS_USBPHY_HSUSBROLE_Msk;   
    
    /* Enable USB PHY */
    SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSUSBEN_Msk;
    for (i=0; i<0x1000; i++);      // delay > 10 us
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
    
    /* Enable HSUSBD module clock */
    CLK_EnableModuleClock(HSUSBD_MODULE);

  
#if 0   
    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);

    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
    
    /* Enable UART2 module clock */ 
    CLK_EnableModuleClock(UART2_MODULE);
    
    /* Select UART2 module clock source as HIRC and UART2 module clock divider as 1 */
    CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HIRC, CLK_CLKDIV4_UART2(1));
    
    /* Enable SPI0 module peripheral clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    
    /* Select PCLK0 as the clock source of SPI0 */
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, MODULE_NoMsk);
    
    
    
    /* Enable TMR1 module clock */
    CLK_EnableModuleClock(TMR1_MODULE);
    
    /* Select TMR1 clock source */
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
    
    /* Enable EPWM1 module clock */
    CLK_EnableModuleClock(EPWM1_MODULE);
    
    /* EPWM clock frequency is set double to PCLK: select EPWM module clock source as PLL */
    CLK_SetModuleClock(EPWM1_MODULE, CLK_CLKSEL2_EPWM1SEL_PLL, (uint32_t)NULL);
    
    /* select HSUSBD */
    SYS->USBPHY &= ~SYS_USBPHY_HSUSBROLE_Msk;   
    
    /* Enable USB PHY */
    SYS->USBPHY = (SYS->USBPHY & ~(SYS_USBPHY_HSUSBROLE_Msk | SYS_USBPHY_HSUSBACT_Msk)) | SYS_USBPHY_HSUSBEN_Msk;
    for (i=0; i<0x1000; i++);      // delay > 10 us
    SYS->USBPHY |= SYS_USBPHY_HSUSBACT_Msk;
    
    /* Enable HSUSBD module clock */
    CLK_EnableModuleClock(HSUSBD_MODULE);
    
    /* Enable SPIM module clock */
    CLK_EnableModuleClock(SPIM_MODULE);
    
    /* Enable I2C0 peripheral clock */
    CLK_EnableModuleClock(I2C0_MODULE);
    
    /* Enable SD0 Module clock */
    CLK_EnableModuleClock(SDH0_MODULE);
    
    /* Enable SD0 peripheral clock */
    CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL0_SDH0SEL_PLL, CLK_CLKDIV0_SDH0(10));

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);
        
    /* Set GPB multi-function pins for UART2 RXD and TXD */
    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB1MFP_Msk | SYS_GPB_MFPL_PB0MFP_Msk);
    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB1MFP_UART2_TXD | SYS_GPB_MFPL_PB0MFP_UART2_RXD);
    
    /* Setup SPI0 multi-function pins */
    SYS->GPA_MFPL |= SYS_GPA_MFPL_PA0MFP_SPI0_MOSI | SYS_GPA_MFPL_PA1MFP_SPI0_MISO | SYS_GPA_MFPL_PA2MFP_SPI0_CLK | SYS_GPA_MFPL_PA3MFP_SPI0_SS;
    
    /* Enable SPI0 clock pin (PA2) schmitt trigger */
    PA->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;
    
    /* Enable SPI0 I/O high slew rate */
    GPIO_SetSlewCtl(PA, 0xF, GPIO_SLEWCTL_HIGH);
    
    /* Set PH0(LED_R), PH1(LED_Y), PH2(LED_G) as LED */
    SYS->GPH_MFPL &= ~(SYS_GPH_MFPL_PH2MFP_Msk | SYS_GPH_MFPL_PH1MFP_Msk | SYS_GPH_MFPL_PH0MFP_Msk);
    SYS->GPH_MFPL |= (SYS_GPH_MFPL_PH2MFP_GPIO | SYS_GPH_MFPL_PH1MFP_GPIO | SYS_GPH_MFPL_PH0MFP_GPIO);
    GPIO_SetMode(PH, (BIT0 | BIT1 | BIT2), GPIO_MODE_OUTPUT);
    LED_R = 1;
    //LED_Y = 1;
    LED_G = 1;
    
    /* Set PG15(SW2), PF11(SW3) as Button*/
    SYS->GPG_MFPH &= ~(SYS_GPG_MFPH_PG15MFP_Msk);
    SYS->GPG_MFPH |= (SYS_GPG_MFPH_PG15MFP_GPIO);
    SYS->GPF_MFPH &= ~(SYS_GPF_MFPH_PF11MFP_Msk);
    SYS->GPF_MFPH |= (SYS_GPF_MFPH_PF11MFP_GPIO);
    GPIO_SetMode(PG, BIT15, GPIO_MODE_INPUT);
    GPIO_SetMode(PF, BIT11, GPIO_MODE_INPUT);
    
    
    /* Set PC12(CH0), PC11(CH1), PC10(CH2), PC9(CH3) as EPWM1 */
    SYS->GPC_MFPH &= ~(SYS_GPC_MFPH_PC12MFP_Msk | SYS_GPC_MFPH_PC11MFP_Msk | SYS_GPC_MFPH_PC10MFP_Msk | SYS_GPC_MFPH_PC9MFP_Msk);
    SYS->GPC_MFPH |= (SYS_GPC_MFPH_PC12MFP_EPWM1_CH0 | SYS_GPC_MFPH_PC11MFP_EPWM1_CH1 | SYS_GPC_MFPH_PC10MFP_EPWM1_CH2 | SYS_GPC_MFPH_PC9MFP_EPWM1_CH3);
 
    /* Init SPIM multi-function pins, MOSI(PC.0), MISO(PC.1), CLK(PC.2), SS(PC.3), D3(PC.4), and D2(PC.5) */
    SYS->GPC_MFPL &= ~(SYS_GPC_MFPL_PC0MFP_Msk | SYS_GPC_MFPL_PC1MFP_Msk | SYS_GPC_MFPL_PC2MFP_Msk |
                       SYS_GPC_MFPL_PC3MFP_Msk | SYS_GPC_MFPL_PC4MFP_Msk | SYS_GPC_MFPL_PC5MFP_Msk);
    SYS->GPC_MFPL |= SYS_GPC_MFPL_PC0MFP_SPIM_MOSI | SYS_GPC_MFPL_PC1MFP_SPIM_MISO |
                     SYS_GPC_MFPL_PC2MFP_SPIM_CLK | SYS_GPC_MFPL_PC3MFP_SPIM_SS |
                     SYS_GPC_MFPL_PC4MFP_SPIM_D3 | SYS_GPC_MFPL_PC5MFP_SPIM_D2;
    PC->SMTEN |= GPIO_SMTEN_SMTEN2_Msk;

    /* Set SPIM I/O pins as high slew rate up to 80 MHz. */
    PC->SLEWCTL = (PC->SLEWCTL & 0xFFFFF000) |
                  (0x1<<GPIO_SLEWCTL_HSREN0_Pos) | (0x1<<GPIO_SLEWCTL_HSREN1_Pos) |
                  (0x1<<GPIO_SLEWCTL_HSREN2_Pos) | (0x1<<GPIO_SLEWCTL_HSREN3_Pos) |
                  (0x1<<GPIO_SLEWCTL_HSREN4_Pos) | (0x1<<GPIO_SLEWCTL_HSREN5_Pos);
                  
    /* Set I2C0 multi-function pins */
    SYS->GPA_MFPL = (SYS->GPA_MFPL & ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk)) |
                    (SYS_GPA_MFPL_PA4MFP_I2C0_SDA | SYS_GPA_MFPL_PA5MFP_I2C0_SCL);
                    
    /* select SD0 multi-function pin */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE7MFP_Msk     | SYS_GPE_MFPL_PE6MFP_Msk     | SYS_GPE_MFPL_PE3MFP_Msk      | SYS_GPE_MFPL_PE2MFP_Msk);
    SYS->GPE_MFPL |=  (SYS_GPE_MFPL_PE7MFP_SD0_CMD | SYS_GPE_MFPL_PE6MFP_SD0_CLK | SYS_GPE_MFPL_PE3MFP_SD0_DAT1 | SYS_GPE_MFPL_PE2MFP_SD0_DAT0);

    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk      | SYS_GPB_MFPL_PB4MFP_Msk);
    SYS->GPB_MFPL |=  (SYS_GPB_MFPL_PB5MFP_SD0_DAT3 | SYS_GPB_MFPL_PB4MFP_SD0_DAT2);

    SYS->GPD_MFPH &= ~(SYS_GPD_MFPH_PD13MFP_Msk);
    SYS->GPD_MFPH |=  (SYS_GPD_MFPH_PD13MFP_SD0_nCD);
    
    /* Set PE4 as DHT11 data pin */
    SYS->GPE_MFPL &= ~(SYS_GPE_MFPL_PE4MFP_Msk);
    SYS->GPE_MFPL |= SYS_GPE_MFPL_PE4MFP_GPIO;
    
    
    /* Set PB9 as ESP-01S Reset */
    GPIO_SetMode(PB, BIT9, GPIO_MODE_OUTPUT);
    PB9 = 1;
#endif 
    /* Lock protected registers */
    SYS_LockReg();
}

#if 0
void ScanUID_LowPwr(void)
{
    uint8_t check;
    
    pcd_lpcd_config_start();
    
    Delay_ms((lpcd.inact_ms+2) * (lpcd.skip_times+1));
    
    check = pcd_lpcd_check();
    
    /*Check Interrupt IRQ source */
	if(check == TRUE)
    {
        uint8_t status;
        
        /*Scan a tag*/
        card_exist:
        BC45_Configuration(CONFIG_14443A);
		status = ScanUID_ISO14443ATagType();
		if(status == _SUCCESS_) goto card_exist;
    }
}
#endif

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{

    SYS_Init();
    
    /* Init Systick */
    Systick_1ms_Init();
    
    /* Init UART to 115200-8n1 for print message */
    //UART_Open(UART0, 115200);
    //UartBuf_Init();
    
    /* Init UART1 to 115200-8n1 for print message */
    UART_Open(UART5, 115200);
    
    /* Init SPI0 for NFC Reader */
    SPI_Init();
    
    /* Init TMR0, set timer frequency to 10 KHz */
//    TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 10000);
//    TIMER_EnableInt(TIMER0);
//    NVIC_EnableIRQ(TMR0_IRQn);
//    TIMER_Start(TIMER0);
    
    /* Init My Application */
    MyApplication_Init();
    
    /* NFC Reader Init */
//    pcd_poweron();
//	pcd_init();
//	pcd_antenna_reset();
    
    /* Connect UART to PC, and open a terminal tool to receive following message */
    DBG_PRINT("NFC_Reader\n");
    
    /* Got no where to go, just loop forever */
    while(1){
        MyApplication_Run();    
        //ScanUID_LowPwr();
    }

}



/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
