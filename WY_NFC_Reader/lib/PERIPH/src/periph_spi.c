#include "macro_utils.h"
#include "periph_spi.h"


//uint8_t txBuffer[65];
//uint8_t rxBuffer[65];


void SPI_Init(void)
{
    /* Configure SPI_FLASH_PORT as a master, MSB first, 8-bit transaction, SPI Mode-0 timing, clock is 4MHz */
    SPI_Open(SPI_NFC_READER_PORT, SPI_MASTER, SPI_MODE_0, 8, 4000000);    
    
    /* Disable auto SS function, control SS signal manually. */
    SPI_DisableAutoSS(SPI_NFC_READER_PORT);
}

/**
  * @brief  Transmit and Receive an amount of data in non-blocking mode.
  * @param  hspi: pointer to a SPI_T structure that contains
  *               the configuration information for SPI module.
  * @param  pT: pointer to transmission data buffer
  * @param  pR: pointer to reception data buffer
  * @param  Size: amount of data to be sent and received
  * @retval StatusTypeDef
  */
StatusTypeDef SPI_TransmitReceive(SPI_T *spi, uint8_t *pT, uint8_t *pR, uint16_t Size, uint32_t Timeout)
{
    StatusTypeDef errorcode = HAL_OK;
    uint32_t tickstart = Get_TickCount();
    
    while (Size--)
    {
        /* 1. Wait until TX FIFO is NOT Full */
        /* Nuvoton Logic: Check SPI_STATUS bit 16 (TXFULL) */
        while(SPI_GET_TX_FIFO_FULL_FLAG(spi))
        {
            if((Timeout != HAL_MAX_DELAY) && ((Get_TickCount() - tickstart) >= Timeout))
            {
                errorcode = HAL_TIMEOUT;
                goto error;
            }
        }

        /* 2. Send byte */
        SPI_WRITE_TX(spi, *pT);
        pT++;

        /* 3. Wait until RX FIFO is NOT Empty */
        /* Nuvoton Logic: Check SPI_STATUS bit 8 (RXEMPTY) */
        while(SPI_GET_RX_FIFO_EMPTY_FLAG(spi))
        {
            if((Timeout != HAL_MAX_DELAY) && ((Get_TickCount() - tickstart) >= Timeout))
            {
                errorcode = HAL_TIMEOUT;
                goto error;
            }
        }

        /* 4. Receive byte */
        *pR = (uint8_t)(spi->RX);
        pR++;
    }

error:
    return errorcode;
}




