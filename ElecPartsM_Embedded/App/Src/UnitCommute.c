#include "UnitCommute.h"
#include "UART_UniHAL.h"
#include "usart.h"

UnitData unitData;
uint8_t UC_FrameBuf[UC_FRAME_MAX_SIZE];
uint8_t UC_LastFrameDirection = 1; // UART1: 1, UART2: 2

uint8_t is_SetID_NextUnitReply = 0;

void ProcessUC_Frame(uint8_t length){
    if(length<2)
        return;
    uint8_t id = UC_FrameBuf[0];
    enum UC_Command cmd = (UC_FrameBuf[1] & 0xF0) >> 4;
    uint8_t msg = (UC_FrameBuf[1] & 0x0F);
    
    if(id != unitData.id && id != 0){
        return;
    }

    if(id == 0){
        UC_Frame frame;
        frame.id = id;
        frame.Cmd_Msg = cmd << 4 + msg;
        frame.OptDataLength = length - 2;
        frame.OptData = &UC_FrameBuf[2];
        frame.SendDirection = UC_Upstream;
        Send_UCFrame(frame);
    }

    switch (cmd)
    {
    case UC_SetID:
        ProcessUC_SetID(id);
        break;
    
        
    default:
        break;
    }
}

static void Send_UCFrame(UC_Frame frame){
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

    if(UC_LastFrameDirection == 1){
        if(frame.SendDirection == UC_Upstream)
            UART_Transmit(&huart1, buf, bufLength);
        else
            UART_Transmit(&huart2, buf, bufLength);
    }else if(UC_LastFrameDirection == 2){
        if(frame.SendDirection == UC_Upstream)
            UART_Transmit(&huart2, buf, bufLength);
        else
            UART_Transmit(&huart1, buf, bufLength);
    }
}

static void ProcessUC_SetID(uint8_t id) {
    unitData.id = id;

    UC_Frame frame;
    frame.id = id-1;
    frame.Cmd_Msg = UC_SetID;
    frame.OptDataLength = 0;
    frame.OptData = NULL;
    frame.SendDirection = UC_Upstream;
    Send_UCFrame(frame);
    

    is_SetID_NextUnitReply = 0;
    frame.id = id+1;
    frame.Cmd_Msg = UC_SetID;
    frame.OptDataLength = 0;
    frame.OptData = NULL;
    frame.SendDirection = UC_Downstream;
    Send_UCFrame(frame);

    /* Use this or hal_delay()
    for(uint16_t ms=0; ms<=100; ms++){
        for(uint32_t count=0; count<20971; count++){
            __asm("NOP");
        }
    }
    */

    HAL_Delay(200);

    if(!is_SetID_NextUnitReply){
        frame.id = id+1;
        frame.Cmd_Msg = UC_CommandDone;
        frame.OptDataLength = 0;
        frame.OptData = NULL;
        frame.SendDirection = UC_Upstream;
        Send_UCFrame(frame);
    }
}