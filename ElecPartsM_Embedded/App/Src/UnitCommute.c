#include "UnitCommute.h"
#include "UART_UniHAL.h"
#include "usart.h"

UnitData_t UnitData;

void Send_UCFrame(UC_Frame_t frame){
    uint8_t buf[UC_FRAME_MAX_SIZE];
    uint8_t bufLength = 2;
    buf[0] = frame.id;
    buf[1] = frame.Cmd_Msg;

    if(frame.OptDataLength > 0 && frame.OptData != NULL){
        for(uint8_t i=0; i<frame.OptDataLength; i++){
            buf[bufLength++] = frame.OptData[i];
            if(bufLength >= UC_FRAME_MAX_SIZE){
                break;
            }
        }
    }

    if(frame.SendDirection == UC_Downstream){
        UART_Transmit(&huart2, buf, bufLength);
    }else if(frame.SendDirection == UC_Upstream){
        UART_Transmit(&huart1, buf, bufLength);
    }
}

void ProcessUC_SetID(uint8_t id) {
    UnitData.id = id;

    UC_Frame_t frame;
    frame.id = id-1;
    frame.Cmd_Msg = UC_SetID;
    frame.OptDataLength = 0;
    frame.OptData = NULL;
    frame.SendDirection = UC_Upstream;
    Send_UCFrame(frame);
    
    /* Use this or hal_delay()
    for(uint16_t ms=0; ms<=100; ms++){
        for(uint32_t count=0; count<20971; count++){
            __asm("NOP");
        }
    }
    */

    HAL_Delay(200);
}