#include "UART_UniHAL.h"
#include "usart.h"

void UART_Transmit(void *handler, uint8_t *data, uint16_t size){
    HAL_UART_Transmit((UART_HandleTypeDef *)handler, data, size, HAL_MAX_DELAY);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    
}