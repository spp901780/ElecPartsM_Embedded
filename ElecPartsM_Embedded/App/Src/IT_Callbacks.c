#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "UnitCommute.h"
#include "WS2812B_Driver.h"

extern WS2812B ledStrip;

uint8_t Uart_ByteReceiveDirection = 0; // 0: not received, 1: received from UART1, 2: received from UART2
uint8_t RxBuf;
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
    if(hspi->Instance == SPI1){
        // WS2812B DMA transmission complete callback
        WS2812B_DMA_IT(&ledStrip);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if(huart->Instance == USART1){
        Uart_ByteReceiveDirection = 1;
        HAL_UART_Receive_IT(huart, &RxBuf, 1);
    }else if(huart->Instance == USART2){
        // UART2 receive complete callback
        Uart_ByteReceiveDirection = 2;
        HAL_UART_Receive_IT(huart, &RxBuf, 1);
    }
}