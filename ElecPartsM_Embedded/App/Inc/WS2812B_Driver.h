#ifndef WS2812B_DRIVER_H
#define WS2812B_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif
#include "main.h"

#define WS2812B_MAX_LED_NUM 15

typedef enum {
    WS2812B_Idle = 0,
    WS2812B_Buffering,
    WS2812B_Transmitting,
    WS2812B_Refreshing
} WS2812B_Status;

typedef enum {
    WS2812B_OK = 0,
    WS2812B_Error
} WS2812B_Result;

typedef struct {
    uint8_t G;
    uint8_t R;
    uint8_t B;
} LED_Color;

typedef struct {
    uint8_t LED_Num;
    WS2812B_Status Status;
    LED_Color LEDs[WS2812B_MAX_LED_NUM];
} WS2812B;

WS2812B_Result WS2812B_SetLEDColor(WS2812B *strip, uint8_t led_index, LED_Color color);
WS2812B_Result WS2812B_SetAllLEDColor(WS2812B *strip, LED_Color color);
WS2812B_Result WS2812B_LitTheLED(WS2812B *strip, uint8_t theLED, LED_Color color);
WS2812B_Result WS2812B_Init(WS2812B *strip);
WS2812B_Result WS2812B_StartRefresh(WS2812B *strip);
WS2812B_Result WS2812B_DMA_IT(WS2812B *strip);

#ifdef __cplusplus
}
#endif

#endif /* WS2812B_DRIVER_H */