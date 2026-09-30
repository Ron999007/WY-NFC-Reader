#ifndef NFC_INTERFACE_H
#define NFC_INTERFACE_H


#define SPI_NFC_READER_PORT     SPI0


void SPI_Init(void);
StatusTypeDef SPI_TransmitReceive(SPI_T *spi, uint8_t *pT, uint8_t *pR, uint16_t Size, uint32_t Timeout);



#endif
