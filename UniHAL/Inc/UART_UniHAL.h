#ifndef __UART_UNIHAL_H__
#define __UART_UNIHAL_H__

#include "main.h"

void UART_Init(void);
void UART_DeInit(void);

void UART_Transmit(void *handler, uint8_t *data, uint16_t size);
void UART_Receive(void *handler, uint8_t *data, uint16_t size);

#endif /* __UART_UNIHAL_H__ */