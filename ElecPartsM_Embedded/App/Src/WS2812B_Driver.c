#include "WS2812B_Driver.h"
#include "spi.h"

WS2812B_Result WS2812B_SetLEDColor(WS2812B *strip, uint8_t led_index, LED_Color color){
    if(strip == NULL || led_index >= WS2812B_MAX_LED_NUM){
        return WS2812B_Error;
    }
    strip->LEDs[led_index] = color;
    return WS2812B_OK;
}

WS2812B_Result WS2812B_SetAllLEDColor(WS2812B *strip, LED_Color color){
    if(strip == NULL || strip->LED_Num > WS2812B_MAX_LED_NUM){
        return WS2812B_Error;
    }
    for(uint8_t i=0; i<strip->LED_Num; i++){
        strip->LEDs[i] = color;
    }
    return WS2812B_OK;
}

WS2812B_Result WS2812B_LitTheLED(WS2812B *strip, uint8_t theLED, LED_Color color){
    if(strip == NULL || strip->LED_Num > WS2812B_MAX_LED_NUM){
        return WS2812B_Error;
    }


    for(uint8_t ledIndex = 0; ledIndex < strip->LED_Num; ledIndex++){
        strip->LEDs[ledIndex] = (ledIndex == theLED) ? color : (LED_Color){0, 0, 0};
    }
    return WS2812B_OK;
}

WS2812B_Result WS2812B_Init(WS2812B *strip){
    if(strip == NULL || strip->LED_Num == 0 || strip->LED_Num > WS2812B_MAX_LED_NUM){
        return WS2812B_Error;
    }
    strip->Status = WS2812B_Idle;
    return WS2812B_OK;
}

WS2812B_Result WS2812B_StartRefresh(WS2812B *strip){
    if(strip == NULL || strip->Status != WS2812B_Idle){
        return WS2812B_Error;
    }
    strip->Status = WS2812B_Buffering;

    uint16_t DMA_Buffer[WS2812B_MAX_LED_NUM * 24] = {0};
    uint16_t *buffer_ptr = DMA_Buffer;
    for(uint8_t led_count = 0; led_count < strip->LED_Num; led_count++){
        for(uint8_t greenBit = 0; greenBit < 8; greenBit++){
            *buffer_ptr = (strip->LEDs[led_count].G & (1 << (7 - greenBit))) ? 0b111111000 : 0b111000000;
            buffer_ptr++;
        }
        for(uint8_t redBit = 0; redBit < 8; redBit++){
            *buffer_ptr = (strip->LEDs[led_count].R & (1 << (7 - redBit))) ? 0b111111000 : 0b111000000;
            buffer_ptr++;
        }
        for(uint8_t blueBit = 0; blueBit < 8; blueBit++){
            *buffer_ptr = (strip->LEDs[led_count].B & (1 << (7 - blueBit))) ? 0b111111000 : 0b111000000;
            buffer_ptr++;
        }
    }

    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)DMA_Buffer, strip->LED_Num * 24);
    strip->Status = WS2812B_Transmitting;
    return WS2812B_OK;
}

WS2812B_Result WS2812B_DMA_IT(WS2812B *strip){
    if(strip == NULL){
        return WS2812B_Error;
    }

    if(strip->Status == WS2812B_Transmitting){
        uint16_t DMA_Buffer[5] = {0};
        if(HAL_OK != HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)DMA_Buffer, sizeof(DMA_Buffer))){
            return WS2812B_Error;
        }
        strip->Status = WS2812B_Refreshing;
        return WS2812B_OK; 
    }else if(strip->Status == WS2812B_Refreshing){
        strip->Status = WS2812B_Idle;
        return WS2812B_OK;
    }else{
        return WS2812B_Error;
    }
}

