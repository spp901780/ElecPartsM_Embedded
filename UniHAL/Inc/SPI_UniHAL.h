#ifndef __SPI_UNIHAL_H__
#define __SPI_UNIHAL_H__

#include "main.h"

void SPI_Init(void);
void SPI_DeInit(void);

void SPI_Transmit(void *handler, uint8_t *data, uint16_t size);
void SPI_Receive(void *handler, uint8_t *data, uint16_t size);

#endif /* SPI_UNIHAL_H */
