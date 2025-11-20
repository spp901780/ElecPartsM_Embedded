#ifndef __UnitCommute_H__
#define __UnitCommute_H__
#include "main.h"

enum UC_Command{
    UC_HighlightPart = 0x1,
    UC_SetLED = 0x2,

    UC_SetID = 0xA,
    UC_ClearID = 0xB,
    UC_CommandDone = 0xC,
    UC_Error = 0xD,
    UC_ExtendCommand = 0xE
};

enum UC_SendDirection{
    UC_Downstream = 0,
    UC_Upstream = 1
};

typedef struct{
    uint8_t id;
} UnitData_t;

typedef struct{
    enum UC_SendDirection SendDirection; // 0: downstream, 1: upstream
    uint8_t id;
    uint8_t Cmd_Msg;
    uint8_t OptDataLength;
    uint8_t *OptData;
} UC_Frame_t;

#define UC_FRAME_MAX_SIZE 10
uint8_t UC_FrameBuf[UC_FRAME_MAX_SIZE];

extern UnitData_t UnitData;

void Send_UCFrame(UC_Frame_t frame);

void ProcessUC_SetID(uint8_t id);


#endif /* __UnitCommute_H__ */