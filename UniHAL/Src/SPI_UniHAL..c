#include "SPI_UniHAL.h"
#include "spi.h"

void SPI_Init(void){

}

void SPI_DeInit(void){

}

void SPI_Transmit(void *handler, uint8_t *data, uint16_t size){
    HAL_SPI_Transmit((SPI_HandleTypeDef *)handler, data, size, HAL_MAX_DELAY);
}

void SPI_Receive(void *handler, uint8_t *data, uint16_t size){
    HAL_SPI_Receive((SPI_HandleTypeDef *)handler, data, size, HAL_MAX_DELAY);
}