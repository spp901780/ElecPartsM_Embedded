/* -----------------------------------------------------------------------------
 * Copyright (c) 2017-2018 Arm Limited
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software. Permission is granted to anyone to use this
 * software for any purpose, including commercial applications, and to alter
 * it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *
 * $Date:        13. November 2018
 * $Revision:    V1.3
 *
 * Driver:       Driver_CAN1
 *
 * Configured:   via STM32CubeMx configuration tool
 * Project:      CAN Driver for STMicroelectronics STM32F0xx
 * --------------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                 Value   CAN Interface
 *   ---------------------                 -----   -------------
 *   Connect to hardware via Driver_CAN# = 1       use bxCAN
 *
 * --------------------------------------------------------------------------
 * Defines used for driver configuration (at compile time):
 *
 *   CAN_CLOCK_TOLERANCE:  defines maximum allowed clock tolerance in 1/1024 steps
 *     - default value:    15 (approx. 1.5 %)
 * -------------------------------------------------------------------------- */

/* History:
 *  Version 1.3
 *    Corrected SetBitrate function to leave Silent and Loopback mode as they were
 *    Corrected SetMode function to clear Silent and Loopback mode when NORMAL mode is activated
 *  Version 1.2
 *    Corrected MessageSend function to only access required data for sending
 *  Version 1.1
 *    Corrected abort message send functionality (IRQ deadloop)
 *  Version 1.0
 *    Initial release
 */

/*! \page stm32f0_can CMSIS-Driver CAN Setup

The CMSIS-Driver CAN requires:
  - Setup of APB1 Clock (maximum frequency is 48 MHz)
  - Configuration of CAN

Valid settings for various evaluation boards are listed in the table below:

Peripheral Resource | STM32091C-EVAL            |
:-------------------|:--------------------------|
CAN Peripheral      | CAN                       |
CAN Mode            | <b>Master Mode</b>        |
RX Pin              | PD0                       |
TX Pin              | PD1                       |

For different boards, refer to the hardware schematics to reflect correct setup values.

The STM32CubeMX configuration for STMicroelectronics STM32091C-EVAL Board with steps for Pinout, Clock, and System Configuration are 
listed below. Enter the values that are marked \b bold.

Pinout tab
----------
  1. Configure peripheral
     - Peripherals \b CAN: Mode=<b>Master Mode</b>

  2. Configure pins PD0 and PD1 as CAN peripheral alternative pins
     - Click in chip diagram on pin \b PD0 and select \b CAN_RX
     - Click in chip diagram on pin \b PD1 and select \b CAN_TX

Clock Configuration tab
-----------------------
  1. Configure APB1 Clock (maximum frequency is 48 MHz)
     - Setup "APB1 peripheral clocks (MHz)" to match application requirements

Configuration tab
-----------------
  1. Under Connectivity open \b CAN Configuration:
     - Parameter Settings: not used
     - User Constants: not used
     - <b>NVIC Settings</b>: enable interrupts
          Interrupt Table                           | Enabled | Preemption Priority
          :-----------------------------------------|:--------|:-------------------
          HDMI-CEC and CAN interrupts..             | \b OFF  | 0

     - <b>GPIO Settings</b>: review settings, no changes required
          Pin Name | Signal on Pin | GPIO output..|  GPIO mode  | GPIO Pull-up/Pull..| Maximum out |  Fast Mode  | User Label
          :--------|:--------------|:-------------|:------------|:-------------------|:------------|:------------|:----------
          PD0      | CAN_RX        | n/a          | Alternate ..| No pull up and no..| Low         | n/a         |.
          PD1      | CAN_TX        | n/a          | Alternate ..| No pull up and no..| Low         | n/a         |.

     Click \b OK to close the CAN Configuration dialog
*/

/*! \cond */

#include "bxCAN_STM32F0xx.h"

#include <string.h>

#include "stm32f0xx.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"


// Externally overridable configuration definitions

// Maximum allowed clock tolerance in 1/1024 steps
#ifndef CAN_CLOCK_TOLERANCE
#define CAN_CLOCK_TOLERANCE             (15U)   // 15/1024 approx. 1.5 %
#endif

// Maximum number of Message Objects used for CAN controller
#define CAN_FILTER_BANK_NUM             (14UL)

#define CAN_RX_OBJ_NUM                  (2U)          // Number of receive objects
#define CAN_TX_OBJ_NUM                  (3U)          // Number of transmit objects
#define CAN_TOT_OBJ_NUM                 (CAN_RX_OBJ_NUM + CAN_TX_OBJ_NUM)


// CAN Driver ******************************************************************

#define ARM_CAN_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,3) // CAN driver version

// Driver Version
static const ARM_DRIVER_VERSION can_driver_version = { ARM_CAN_API_VERSION, ARM_CAN_DRV_VERSION };

// Driver Capabilities
static const ARM_CAN_CAPABILITIES can_driver_capabilities = {
  CAN_TOT_OBJ_NUM,      // Number of CAN Objects available
  1U,                   // Supports reentrant calls to ARM_CAN_MessageSend, ARM_CAN_MessageRead, ARM_CAN_ObjectConfigure and abort message sending used by ARM_CAN_Control.
  0U,                   // Does not support CAN with flexible data-rate mode (CAN_FD)
  0U,                   // Does not support restricted operation mode
  1U,                   // Supports bus monitoring mode
  1U,                   // Supports internal loopback mode
  1U                    // Supports external loopback mode
#if (defined(ARM_CAN_API_VERSION) && (ARM_CAN_API_VERSION >= 0x101U))
, 0U
#endif
};

// Object Capabilities
static const ARM_CAN_OBJ_CAPABILITIES can_object_capabilities_rx = {
  0U,                   // Object does not support transmission
  1U,                   // Object supports reception
  0U,                   // Object does not support RTR reception and automatic Data transmission
  0U,                   // Object does not support RTR transmission and automatic Data reception
  1U,                   // Object allows assignment of multiple filters to it
  1U,                   // Object supports exact identifier filtering
  0U,                   // Object does not support range identifier filtering
  1U,                   // Object supports mask identifier filtering
  3U                    // Object can buffer 3 messages
#if (defined(ARM_CAN_API_VERSION) && (ARM_CAN_API_VERSION >= 0x101U))
, 0U
#endif
};
static const ARM_CAN_OBJ_CAPABILITIES can_object_capabilities_tx = {
  1U,                   // Object supports transmission
  0U,                   // Object does not support reception
  0U,                   // Object does not support RTR reception and automatic Data transmission
  0U,                   // Object does not support RTR transmission and automatic Data reception
  0U,                   // Object does not allow assignment of multiple filters to it
  0U,                   // Object does not support exact identifier filtering
  0U,                   // Object does not support range identifier filtering
  0U,                   // Object does not support mask identifier filtering
  1U                    // Object can only buffer 1 message
#if (defined(ARM_CAN_API_VERSION) && (ARM_CAN_API_VERSION >= 0x101U))
, 0U
#endif
};

#define CAN_FRx_32BIT_IDE               ((uint32_t)1U <<  2)
#define CAN_FRx_32BIT_RTR               ((uint32_t)1U <<  1)
#define CAN_FRx_16BIT_L_IDE             ((uint32_t)1U <<  3)
#define CAN_FRx_16BIT_L_RTR             ((uint32_t)1U <<  4)
#define CAN_FRx_16BIT_H_IDE             ((uint32_t)1U << 19)
#define CAN_FRx_16BIT_H_RTR             ((uint32_t)1U << 20)

typedef enum _CAN_FILTER_TYPE {
  CAN_FILTER_TYPE_EXACT_ID    = 0U,
  CAN_FILTER_TYPE_MASKABLE_ID = 1U
} CAN_FILTER_TYPE;

extern CAN_HandleTypeDef hcan;

static CAN_TypeDef * const ptr_CAN = (CAN_TypeDef *)CAN;

// Local variables and structures
static uint8_t                     can_driver_powered;
static uint8_t                     can_driver_initialized;
static uint8_t                     can_obj_cfg [CAN_TOT_OBJ_NUM];
static ARM_CAN_SignalUnitEvent_t   CAN_SignalUnitEvent;
static ARM_CAN_SignalObjectEvent_t CAN_SignalObjectEvent;


// Helper Functions

/**
  \fn          int32_t CAN_AddFilter (uint32_t obj_idx, CAN_FILTER_TYPE filter_type, uint32_t id, uint32_t mask)
  \brief       Add receive filter for specified id or id with mask.
  \param[in]   obj_idx      Receive object index assignment
  \param[in]   filter_type  Type of filter to add
                 - CAN_FILTER_TYPE_EXACT_ID:    exact id filter (id only)
                 - CAN_FILTER_TYPE_MASKABLE_ID: maskable filter (id and mask)
  \param[in]   id           Identifier
  \param[in]   mask         Identifier Mask
  \return      execution status
*/
static int32_t CAN_AddFilter (uint32_t obj_idx, CAN_FILTER_TYPE filter_type, uint32_t id, uint32_t mask) {
  uint32_t fa1r, frx, fry, msk;
  uint8_t  bank;
  int32_t  status;

  if (obj_idx >= CAN_RX_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }


  status = ARM_DRIVER_OK;
  ptr_CAN->FMR |= CAN_FMR_FINIT;                                        // Enter filter initialization mode

  bank = 0U;
  fa1r = ptr_CAN->FA1R;
  msk  = (uint32_t)1U << bank;

  if ((id & ARM_CAN_ID_IDE_Msk) != 0U) {                                // Extended Identifier
    frx = (id << 3) | CAN_FRx_32BIT_IDE;                                // id + IDE
    switch (filter_type) {
      case CAN_FILTER_TYPE_EXACT_ID:
        fry = (id << 3) | CAN_FRx_32BIT_IDE | CAN_FRx_32BIT_RTR;        // id + IDE + RTR
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if ((fa1r & msk) == 0U) {                                     // If filter is not active
            ptr_CAN->FM1R |=  msk;                                      // Select identifier list mode
            ptr_CAN->FS1R |=  msk;                                      // Select single 32-bit scale configuration
            if ((obj_idx & 1U) != 0U) {
              ptr_CAN->FFA1R |=  msk;                                   // Assign to FIFO1
            } else {
              ptr_CAN->FFA1R &= ~msk;                                   // Assign to FIFO0
            }
            ptr_CAN->sFilterRegister[bank].FR1 = frx;                   // id + IDE
            ptr_CAN->sFilterRegister[bank].FR2 = fry;                   // id + IDE + RTR
            break;
          }
          msk <<= 1U;
          bank++;
        }
        break;
      case CAN_FILTER_TYPE_MASKABLE_ID:
        fry = (mask << 3) | CAN_FRx_32BIT_IDE;                          // mask + IDE
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if ((fa1r & msk) == 0U) {                                     // If filter is not active
            ptr_CAN->FM1R &= ~msk;                                      // Select identifier mask mode
            ptr_CAN->FS1R |=  msk;                                      // Select single 32-bit scale configuration
            if ((obj_idx & 1U) != 0U) {
              ptr_CAN->FFA1R |=  msk;                                   // Assign to FIFO1
            } else {
              ptr_CAN->FFA1R &= ~msk;                                   // Assign to FIFO0
            }
            ptr_CAN->sFilterRegister[bank].FR1 = frx;                   // id + IDE
            ptr_CAN->sFilterRegister[bank].FR2 = fry;                   // mask + IDE
            break;
          }
          msk <<= 1U;
          bank++;
        }
        break;
      default:
        status = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  } else {                                                              // Standard Identifier
    switch (filter_type) {
      case CAN_FILTER_TYPE_EXACT_ID:
        frx = ((id & 0xFFFFU) <<  5) |                                  // Low 16 bits = id
              ((id & 0xFFFFU) << 21) | CAN_FRx_16BIT_H_RTR;             // High 16 bits = id + RTR
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if ((fa1r & msk) == 0U) {                                     // If filter is not active
            ptr_CAN->FM1R |=  msk;                                      // Select identifier list mode
            ptr_CAN->FS1R &= ~msk;                                      // Select dual 16-bit scale configuration
            if ((obj_idx & 1U) != 0U) {
              ptr_CAN->FFA1R |=  msk;                                   // Assign to FIFO1
            } else {
              ptr_CAN->FFA1R &= ~msk;                                   // Assign to FIFO0
            }
            ptr_CAN->sFilterRegister[bank].FR1 = frx;
            ptr_CAN->sFilterRegister[bank].FR2 = 0xFFFFFFFFU;           // Disable unused slots
            break;
          } else if  (((ptr_CAN->FM1R & msk) != 0U) &&                  // If identifier list mode
                      ((ptr_CAN->FS1R & msk) == 0U) &&                  // If dual 16-bit scale configuration
                     (((ptr_CAN->FFA1R>>bank)&1U) == obj_idx)) {        // If bank has same FIFO assignment as requested
            if         (ptr_CAN->sFilterRegister[bank].FR1==0xFFFFFFFFU){// If n and n+1 entry are not used
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR1 = frx;                 // id and id + RTR
              break;
            } else if  (ptr_CAN->sFilterRegister[bank].FR2==0xFFFFFFFFU){// If n+2 and n+3 entry are not used
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR2 = frx;                 // id and id + RTR
              break;
            }
          }
          msk <<= 1U;
          bank++;
        }
        break;
      case CAN_FILTER_TYPE_MASKABLE_ID:
        frx = ((id   & 0xFFFFU) <<  5) |                                // Low 16 bits = id
              ((mask & 0xFFFFU) << 21) ;                                // High 16 bits = mask
        if ((mask & ARM_CAN_ID_IDE_Msk) != 0U) {                        // If IDE masking enabled
          frx |= CAN_FRx_16BIT_H_RTR;                                   // High 16 bits = mask + IDE
        }
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if ((fa1r & msk) == 0U) {                                     // If filter is not active
            ptr_CAN->FM1R &= ~msk;                                      // Select identifier mask mode
            ptr_CAN->FS1R &= ~msk;                                      // Select dual 16-bit scale configuration
            if ((obj_idx & 1U) != 0U) {
              ptr_CAN->FFA1R |=  msk;                                   // Assign to FIFO1
            } else {
              ptr_CAN->FFA1R &= ~msk;                                   // Assign to FIFO0
            }
            ptr_CAN->sFilterRegister[bank].FR1 = frx;
            ptr_CAN->sFilterRegister[bank].FR2 = 0xFFFFFFFFU;           // Disable unused slots
            break;
          } else if ((((ptr_CAN->FM1R | ptr_CAN->FS1R)&msk)==0U) &&     // If identifier mask mode and dual 16-bit scale configuration
                     (((ptr_CAN->FFA1R >> bank)&1U)==obj_idx)) {        // If bank has same FIFO assignment as requested
            if         (ptr_CAN->sFilterRegister[bank].FR1==0xFFFFFFFFU){// If n and n+1 entry are not used
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR1 = frx;                 // id and mask
              break;
            } else if  (ptr_CAN->sFilterRegister[bank].FR2==0xFFFFFFFFU){// If n+2 and n+3 entry are not used
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR2 = frx;                 // id and mask
              break;
            }
          }
          msk <<= 1U;
          bank++;
        }
        break;
      default:
        status = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  }

  if (status == ARM_DRIVER_OK) {
    ptr_CAN->FA1R |= msk;                                               // Put filter in active mode
  }
  ptr_CAN->FMR  &= ~CAN_FMR_FINIT;                                      // Exit filter initialization mode

  return status;
}

/**
  \fn          int32_t CAN_RemoveFilter (uint32_t obj_idx, CAN_FILTER_TYPE filter_type, uint32_t id, uint32_t mask)
  \brief       Remove receive filter for specified id or id with mask.
  \param[in]   obj_idx      Receive object index assignment
  \param[in]   filter_type  Type of filter to remove
                 - CAN_FILTER_TYPE_EXACT_ID:    exact id filter (id only)
                 - CAN_FILTER_TYPE_MASKABLE_ID: maskable filter (id and mask)
  \param[in]   id           Identifier
  \param[in]   mask         Identifier Mask
  \return      execution status
*/
static int32_t CAN_RemoveFilter (uint32_t obj_idx, CAN_FILTER_TYPE filter_type, uint32_t id, uint32_t mask) {
  uint32_t     fa1r, frx, fry, msk;
  int32_t      status;
  uint8_t      bank;

  if (obj_idx >= CAN_RX_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }

  status = ARM_DRIVER_OK;
  ptr_CAN->FMR |= CAN_FMR_FINIT;                                        // Enter filter initialization mode

  bank = 0U;
  fa1r = ptr_CAN->FA1R;
  msk  = (uint32_t)1U << bank;

  if ((id & ARM_CAN_ID_IDE_Msk) != 0U) {                                // Extended Identifier
    frx = (id << 3) | CAN_FRx_32BIT_IDE;                                // id + IDE
    switch (filter_type) {
      case CAN_FILTER_TYPE_EXACT_ID:
        fry = (id << 3) | CAN_FRx_32BIT_IDE | CAN_FRx_32BIT_RTR;        // id + IDE + RTR
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if (((fa1r & msk) != 0U) &&                                   // If filter is active
             (((ptr_CAN->FM1R&ptr_CAN->FS1R)&msk)!=0U) &&               // If identifier list mode and single 32-bit scale configuration
             (((ptr_CAN->FFA1R >> bank) & 1U) == obj_idx) &&            // If bank has same FIFO assignment as requested
             (ptr_CAN->sFilterRegister[bank].FR1 == frx) &&
             (ptr_CAN->sFilterRegister[bank].FR2 == fry)) {
            ptr_CAN->FA1R &= ~msk;                                      // Put filter in inactive mode
            ptr_CAN->sFilterRegister[bank].FR1 = 0xFFFFFFFFU;
            ptr_CAN->sFilterRegister[bank].FR2 = 0xFFFFFFFFU;
            break;
          }
          msk <<= 1U;
          bank++;
       }
        break;
      case CAN_FILTER_TYPE_MASKABLE_ID:
        fry = (mask << 3) | CAN_FRx_32BIT_IDE;                          // mask + IDE
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if (((fa1r & msk) != 0U) &&                                   // If filter is active
              ((ptr_CAN->FM1R & msk) == 0U) &&                          // If identifier mask mode
              ((ptr_CAN->FS1R & msk) != 0U) &&                          // If single 32-bit scale configuration
             (((ptr_CAN->FFA1R >> bank) & 1U) == obj_idx)  &&           // If bank has same FIFO assignment as requested
             (ptr_CAN->sFilterRegister[bank].FR1 == frx) &&
             (ptr_CAN->sFilterRegister[bank].FR2 == fry)) {
            ptr_CAN->FA1R &= ~msk;                                      // Put filter in inactive mode
            ptr_CAN->sFilterRegister[bank].FR1 = 0xFFFFFFFFU;
            ptr_CAN->sFilterRegister[bank].FR2 = 0xFFFFFFFFU;
            break;
          }
          msk <<= 1U;
          bank++;
        }
        break;
      default:
        status = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  } else {                                                              // Standard Identifier
    switch (filter_type) {
      case CAN_FILTER_TYPE_EXACT_ID:
        frx = ((id & 0xFFFFU) <<  5) |                                  // Low 16 bits = id
              ((id & 0xFFFFU) << 21) | CAN_FRx_16BIT_H_RTR;             // High 16 bits = id + RTR
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if (((fa1r & msk) != 0U)&&                                    // If filter is active
              ((ptr_CAN->FM1R & msk) != 0U) &&                          // If identifier list mode
              ((ptr_CAN->FS1R & msk) == 0U) &&                          // If dual 16-bit scale configuration
             (((ptr_CAN->FFA1R >> bank) & 1U) == obj_idx)) {            // If bank has same FIFO assignment as requested
            if        (ptr_CAN->sFilterRegister[bank].FR1==frx){
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR1 = 0xFFFFFFFFU;
              break;
            } else if (ptr_CAN->sFilterRegister[bank].FR2==frx){
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR2 = 0xFFFFFFFFU;
              break;
            }
          }
          msk <<= 1U;
          bank++;
        }
        break;
      case CAN_FILTER_TYPE_MASKABLE_ID:
        frx = ((id   & 0xFFFFU) <<  5) |                                // Low 16 bits = id
              ((mask & 0xFFFFU) << 21) ;                                // High 16 bits = mask
        if ((mask & ARM_CAN_ID_IDE_Msk) != 0U) {                        // If IDE masking enabled
          frx |= CAN_FRx_16BIT_H_IDE;                                   // High 16 bits = mask + IDE
        }
        while (bank <= CAN_FILTER_BANK_NUM) {                           // Find empty place for id
          if (bank == CAN_FILTER_BANK_NUM) {                            // If no free found exit
            status = ARM_DRIVER_ERROR;
            break;
          }
          if (((fa1r & msk) != 0U) &&                                   // If filter is active
             (((ptr_CAN->FM1R|ptr_CAN->FS1R)&msk)==0U) &&               // If identifier mask mode and dual 16-bit scale configuration
             (((ptr_CAN->FFA1R >> bank) & 1U) == obj_idx)) {            // If bank has same FIFO assignment as requested
            if        (ptr_CAN->sFilterRegister[bank].FR1==frx){
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR1 = 0xFFFFFFFFU;
              break;
            } else if (ptr_CAN->sFilterRegister[bank].FR2==frx){
              ptr_CAN->FA1R &= ~msk;                                    // Put filter in inactive mode
              ptr_CAN->sFilterRegister[bank].FR2 = 0xFFFFFFFFU;
              break;
            }
          }
          msk <<= 1U;
          bank++;
        }
        break;
      default:
        status = ARM_DRIVER_ERROR_PARAMETER;
        break;
    }
  }

  if ((status == ARM_DRIVER_OK)                            && 
     ((ptr_CAN->sFilterRegister[bank].FR1 != 0xFFFFFFFFU)  || 
      (ptr_CAN->sFilterRegister[bank].FR2 != 0xFFFFFFFFU))) {
    ptr_CAN->FA1R |= msk;                                               // Put filter in active mode
  }
  ptr_CAN->FMR  &= ~CAN_FMR_FINIT;                                      // Exit filter initialization mode

  return status;
}

// CAN Driver Functions

/**
  \fn          ARM_DRIVER_VERSION CAN_GetVersion (void)
  \brief       Get driver version.
  \return      ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION CAN_GetVersion (void) { return can_driver_version; }

/**
  \fn          ARM_CAN_CAPABILITIES CAN_GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      ARM_CAN_CAPABILITIES
*/
static ARM_CAN_CAPABILITIES CAN_GetCapabilities (void) { return can_driver_capabilities; }

/**
  \fn          int32_t CAN_Initialize (ARM_CAN_SignalUnitEvent_t   cb_unit_event,
                                       ARM_CAN_SignalObjectEvent_t cb_object_event)
  \brief       Initialize CAN interface and register signal (callback) functions.
  \param[in]   cb_unit_event   Pointer to ARM_CAN_SignalUnitEvent callback function
  \param[in]   cb_object_event Pointer to ARM_CAN_SignalObjectEvent callback function
  \return      execution status
*/
static int32_t CAN_Initialize (ARM_CAN_SignalUnitEvent_t   cb_unit_event,
                               ARM_CAN_SignalObjectEvent_t cb_object_event) {

  if (can_driver_initialized != 0U) { return ARM_DRIVER_OK; }

  CAN_SignalUnitEvent   = cb_unit_event;
  CAN_SignalObjectEvent = cb_object_event;

  hcan.Instance = CAN;

  can_driver_initialized = 1U;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t CAN_Uninitialize (void)
  \brief       De-initialize CAN interface.
  \return      execution status
*/
static int32_t CAN_Uninitialize (void) {

  hcan.Instance          = NULL;

  can_driver_initialized = 0U;

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t CAN_PowerControl (ARM_POWER_STATE state)
  \brief       Control CAN interface power.
  \param[in]   state  Power state
                 - ARM_POWER_OFF :  power off: no operation possible
                 - ARM_POWER_LOW :  low power mode: retain state, detect and signal wake-up events
                 - ARM_POWER_FULL : power on: full operation at maximum performance
  \return      execution status
*/
static int32_t CAN_PowerControl (ARM_POWER_STATE state) {
  uint32_t msk;
  uint32_t tout_cnt;
  uint8_t  bank;

  switch (state) {
    case ARM_POWER_OFF:
      can_driver_powered = 0U;
      NVIC_DisableIRQ(CEC_CAN_IRQn);
      if (hcan.Instance != NULL) {
        HAL_CAN_DeInit (&hcan);
      }
      memset((void *)&can_obj_cfg[0], 0, CAN_TOT_OBJ_NUM);
      break;

    case ARM_POWER_FULL:
      if (can_driver_initialized == 0U) { return ARM_DRIVER_ERROR; }
      if (can_driver_powered     != 0U) { return ARM_DRIVER_OK;    }
      if (hcan.Instance != NULL) {
        HAL_CAN_MspInit(&hcan);
      }

      ptr_CAN->MCR = CAN_MCR_RESET;             // Reset CAN controller
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, for reset to end
      while ((ptr_CAN->MCR & CAN_MCR_RESET) != 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }

      // Initialize filter banks
      bank = 0U;
      msk  = ((uint32_t)1U << CAN_FILTER_BANK_NUM) - (uint32_t)1U;
      ptr_CAN->FMR  |=  CAN_FMR_FINIT;          // Enter filter initialization mode
      ptr_CAN->FA1R &= ~msk;                    // Put all filters in inactive mode
      ptr_CAN->FM1R &= ~msk;                    // Initialize all filters mode
      ptr_CAN->FS1R &= ~msk;                    // Initialize all filters scale configuration
      while (bank <= CAN_FILTER_BANK_NUM) {     // Go through all banks
        if (bank == CAN_FILTER_BANK_NUM) { break; }
        ptr_CAN->sFilterRegister[bank].FR1 = 0U;
        ptr_CAN->sFilterRegister[bank].FR2 = 0U;
        bank++;
      }

      memset((void *)&can_obj_cfg[0], 0, CAN_TOT_OBJ_NUM);

      ptr_CAN->IER =   CAN_IER_TMEIE  |         // Enable Interrupts
                       CAN_IER_FMPIE0 |
                       CAN_IER_FOVIE0 |
                       CAN_IER_FMPIE1 |
                       CAN_IER_FOVIE1 |
                       CAN_IER_EWGIE  |
                       CAN_IER_EPVIE  |
                       CAN_IER_BOFIE  |
                       CAN_IER_ERRIE  ;

      can_driver_powered = 1U;

      if ((CAN_SignalUnitEvent != NULL) || (CAN_SignalObjectEvent != NULL)) {
        NVIC_ClearPendingIRQ (CEC_CAN_IRQn);
        NVIC_EnableIRQ       (CEC_CAN_IRQn);
      }
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          uint32_t CAN_GetClock (void)
  \brief       Retrieve CAN base clock frequency.
  \return      base clock frequency
*/
static uint32_t CAN_GetClock (void) {
  return HAL_RCC_GetPCLK1Freq();
}

/**
  \fn          int32_t CAN_SetBitrate (ARM_CAN_BITRATE_SELECT select, uint32_t bitrate, uint32_t bit_segments)
  \brief       Set bitrate for CAN interface.
  \param[in]   select       Bitrate selection
                 - ARM_CAN_BITRATE_NOMINAL : nominal (flexible data-rate arbitration) bitrate
                 - ARM_CAN_BITRATE_FD_DATA : flexible data-rate data bitrate
  \param[in]   bitrate      Bitrate
  \param[in]   bit_segments Bit segments settings
  \return      execution status
*/
static int32_t CAN_SetBitrate (ARM_CAN_BITRATE_SELECT select, uint32_t bitrate, uint32_t bit_segments) {
  uint32_t     mcr, sjw, prop_seg, phase_seg1, phase_seg2, pclk, brp, tq_num, tmp, tout_cnt;

  if (select != ARM_CAN_BITRATE_NOMINAL) { return ARM_CAN_INVALID_BITRATE_SELECT; }
  if (can_driver_powered == 0U)          { return ARM_DRIVER_ERROR;               }

  prop_seg   = (bit_segments & ARM_CAN_BIT_PROP_SEG_Msk  ) >> ARM_CAN_BIT_PROP_SEG_Pos;
  phase_seg1 = (bit_segments & ARM_CAN_BIT_PHASE_SEG1_Msk) >> ARM_CAN_BIT_PHASE_SEG1_Pos;
  phase_seg2 = (bit_segments & ARM_CAN_BIT_PHASE_SEG2_Msk) >> ARM_CAN_BIT_PHASE_SEG2_Pos;
  sjw        = (bit_segments & ARM_CAN_BIT_SJW_Msk       ) >> ARM_CAN_BIT_SJW_Pos;

  if (((prop_seg + phase_seg1) < 1U) || ((prop_seg + phase_seg1) > 16U)) { return ARM_CAN_INVALID_BIT_PROP_SEG;   }
  if (( phase_seg2             < 1U) || ( phase_seg2             >  8U)) { return ARM_CAN_INVALID_BIT_PHASE_SEG2; }
  if (( sjw                    < 1U) || ( sjw                    >  4U)) { return ARM_CAN_INVALID_BIT_SJW;        }

  tq_num = 1U + prop_seg + phase_seg1 + phase_seg2;
  pclk   = CAN_GetClock ();           if (pclk == 0U)  { return ARM_DRIVER_ERROR;        }
  brp    = pclk / (tq_num * bitrate); if (brp > 1024U) { return ARM_CAN_INVALID_BITRATE; }
  tmp    = (brp * tq_num * bitrate);
  if        (pclk > tmp) {
    if   ((((pclk - tmp) * 1024U) / pclk) > CAN_CLOCK_TOLERANCE) { return ARM_CAN_INVALID_BITRATE; }
  } else if (pclk < tmp) {
    if   ((((tmp - pclk) * 1024U) / pclk) > CAN_CLOCK_TOLERANCE) { return ARM_CAN_INVALID_BITRATE; }
  }

  mcr = ptr_CAN->MCR;
  ptr_CAN->MCR = CAN_MCR_INRQ;                  // Activate initialization mode
  tout_cnt = SystemCoreClock / 64U;             // Wait, with timeout, for initialization to activate
  while ((ptr_CAN->MSR & CAN_MSR_INAK) == 0U) {
    if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
  }

  ptr_CAN->BTR = (ptr_CAN->BTR & (CAN_BTR_LBKM | CAN_BTR_SILM)) | ((brp - 1U) & CAN_BTR_BRP) | ((sjw - 1U) << 24) | ((phase_seg2 - 1U) << 20) | ((prop_seg + phase_seg1 - 1U) << 16);
  ptr_CAN->MCR =  mcr;                          // Return to previous mode

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t CAN_SetMode (ARM_CAN_MODE mode)
  \brief       Set operating mode for CAN interface.
  \param[in]   mode   Operating mode
                 - ARM_CAN_MODE_INITIALIZATION :    initialization mode
                 - ARM_CAN_MODE_NORMAL :            normal operation mode
                 - ARM_CAN_MODE_RESTRICTED :        restricted operation mode
                 - ARM_CAN_MODE_MONITOR :           bus monitoring mode
                 - ARM_CAN_MODE_LOOPBACK_INTERNAL : loopback internal mode
                 - ARM_CAN_MODE_LOOPBACK_EXTERNAL : loopback external mode
  \return      execution status
*/
static int32_t CAN_SetMode (ARM_CAN_MODE mode) {
  uint32_t     event, tout_cnt;

  event = 0U;
  switch (mode) {
    case ARM_CAN_MODE_INITIALIZATION:
      ptr_CAN->FMR |=  CAN_FMR_FINIT;           // Filter initialization mode
      ptr_CAN->MCR  =  CAN_MCR_INRQ;            // Enter initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to enter initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) == 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      event = ARM_CAN_EVENT_UNIT_BUS_OFF;
      break;

    case ARM_CAN_MODE_NORMAL:
      ptr_CAN->MCR |=  CAN_MCR_INRQ;            // Enter initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to enter initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) == 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      ptr_CAN->BTR &=~(CAN_BTR_LBKM | CAN_BTR_SILM);
      ptr_CAN->MCR  =  CAN_MCR_ABOM |           // Activate automatic bus-off
                       CAN_MCR_AWUM ;           // Enable automatic wakeup mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to exit initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) != 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      ptr_CAN->FMR &= ~CAN_FMR_FINIT;           // Filter active mode
      event = ARM_CAN_EVENT_UNIT_ACTIVE;
      break;

    case ARM_CAN_MODE_RESTRICTED:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_CAN_MODE_MONITOR:
      ptr_CAN->MCR |=  CAN_MCR_INRQ;            // Enter initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to enter initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) == 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      ptr_CAN->BTR &= ~CAN_BTR_LBKM;            // Deactivate loopback
      ptr_CAN->BTR |=  CAN_BTR_SILM;            // Activate silent
      ptr_CAN->MCR &= ~CAN_MCR_INRQ;            // Deactivate initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to exit initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) != 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      event = ARM_CAN_EVENT_UNIT_PASSIVE;
      break;

    case ARM_CAN_MODE_LOOPBACK_INTERNAL:
      ptr_CAN->MCR |=  CAN_MCR_INRQ;            // Enter initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to enter initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) == 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      ptr_CAN->BTR |=  CAN_BTR_LBKM;            // Activate loopback
      ptr_CAN->BTR |=  CAN_BTR_SILM;            // Activate silent
      ptr_CAN->MCR &= ~CAN_MCR_INRQ;            // Deactivate initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to exit initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) != 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      event = ARM_CAN_EVENT_UNIT_PASSIVE;
      break;

    case ARM_CAN_MODE_LOOPBACK_EXTERNAL:
      ptr_CAN->MCR |=  CAN_MCR_INRQ;            // Enter initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to enter initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) == 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      ptr_CAN->BTR &= ~CAN_BTR_SILM;            // Deactivate silent
      ptr_CAN->BTR |=  CAN_BTR_LBKM;            // Activate loopback
      ptr_CAN->MCR &= ~CAN_MCR_INRQ;            // Deactivate initialization mode
      tout_cnt = SystemCoreClock / 64U;         // Wait, with timeout, to exit initialization mode
      while ((ptr_CAN->MSR & CAN_MSR_INAK) != 0U) {
        if (tout_cnt-- == 0U) { return ARM_DRIVER_ERROR; }
      }
      event = ARM_CAN_EVENT_UNIT_ACTIVE;
      break;

    default:
      return ARM_DRIVER_ERROR_PARAMETER;
  }
  if ((CAN_SignalUnitEvent != NULL) && (event != 0U)) { CAN_SignalUnitEvent(event); }

  return ARM_DRIVER_OK;
}

/**
  \fn          ARM_CAN_OBJ_CAPABILITIES CAN_ObjectGetCapabilities (uint32_t obj_idx)
  \brief       Retrieve capabilities of an object.
  \param[in]   obj_idx  Object index
  \return      ARM_CAN_OBJ_CAPABILITIES
*/
static ARM_CAN_OBJ_CAPABILITIES CAN_ObjectGetCapabilities (uint32_t obj_idx) {
  ARM_CAN_OBJ_CAPABILITIES obj_cap_null;

  if (obj_idx >= CAN_TOT_OBJ_NUM) {
    memset ((void *)&obj_cap_null, 0, sizeof(ARM_CAN_OBJ_CAPABILITIES));
    return obj_cap_null;
  }

  if (obj_idx >= CAN_RX_OBJ_NUM) {
    return can_object_capabilities_tx;
  } else {
    return can_object_capabilities_rx;
  }
}

/**
  \fn          int32_t CAN_ObjectSetFilter (uint32_t obj_idx, ARM_CAN_FILTER_OPERATION operation, uint32_t id, uint32_t arg)
  \brief       Add or remove filter for message reception.
  \param[in]   obj_idx      Object index of object that filter should be or is assigned to
  \param[in]   operation    Operation on filter
                 - ARM_CAN_FILTER_ID_EXACT_ADD :       add    exact id filter
                 - ARM_CAN_FILTER_ID_EXACT_REMOVE :    remove exact id filter
                 - ARM_CAN_FILTER_ID_RANGE_ADD :       add    range id filter
                 - ARM_CAN_FILTER_ID_RANGE_REMOVE :    remove range id filter
                 - ARM_CAN_FILTER_ID_MASKABLE_ADD :    add    maskable id filter
                 - ARM_CAN_FILTER_ID_MASKABLE_REMOVE : remove maskable id filter
  \param[in]   id           ID or start of ID range (depending on filter type)
  \param[in]   arg          Mask or end of ID range (depending on filter type)
  \return      execution status
*/
static int32_t CAN_ObjectSetFilter (uint32_t obj_idx, ARM_CAN_FILTER_OPERATION operation, uint32_t id, uint32_t arg) {
  int32_t      status;

  if (obj_idx >= CAN_RX_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered == 0U)  { return ARM_DRIVER_ERROR;           }

  ptr_CAN->FMR |=  CAN_FMR_FINIT;       // Filter initialization mode

  switch (operation) {
    case ARM_CAN_FILTER_ID_EXACT_ADD:
      status = CAN_AddFilter   (obj_idx, CAN_FILTER_TYPE_EXACT_ID,    id,  0U);
      break;
    case ARM_CAN_FILTER_ID_MASKABLE_ADD:
      status = CAN_AddFilter   (obj_idx, CAN_FILTER_TYPE_MASKABLE_ID, id, arg);
      break;
    case ARM_CAN_FILTER_ID_EXACT_REMOVE:
      status = CAN_RemoveFilter(obj_idx, CAN_FILTER_TYPE_EXACT_ID,    id,  0U);
      break;
    case ARM_CAN_FILTER_ID_MASKABLE_REMOVE:
      status = CAN_RemoveFilter(obj_idx, CAN_FILTER_TYPE_MASKABLE_ID, id, arg);
      break;
    case ARM_CAN_FILTER_ID_RANGE_ADD:
    case ARM_CAN_FILTER_ID_RANGE_REMOVE:
    default:
      status = ARM_DRIVER_ERROR_UNSUPPORTED;
      break;
  }
  ptr_CAN->FMR &= ~CAN_FMR_FINIT;       // Filter active mode

  return status;
}

/**
  \fn          int32_t CAN_ObjectConfigure (uint32_t obj_idx, ARM_CAN_OBJ_CONFIG obj_cfg)
  \brief       Configure object.
  \param[in]   obj_idx  Object index
  \param[in]   obj_cfg  Object configuration state
                 - ARM_CAN_OBJ_INACTIVE :       deactivate object
                 - ARM_CAN_OBJ_RX :             configure object for reception
                 - ARM_CAN_OBJ_TX :             configure object for transmission
                 - ARM_CAN_OBJ_RX_RTR_TX_DATA : configure object that on RTR reception automatically transmits Data Frame
                 - ARM_CAN_OBJ_TX_RTR_RX_DATA : configure object that transmits RTR and automatically receives Data Frame
  \return      execution status
*/
static int32_t CAN_ObjectConfigure (uint32_t obj_idx, ARM_CAN_OBJ_CONFIG obj_cfg) {

  if (obj_idx >= CAN_TOT_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered == 0U)   { return ARM_DRIVER_ERROR;           }

  switch (obj_cfg) {
    case ARM_CAN_OBJ_INACTIVE:
      can_obj_cfg[obj_idx] = ARM_CAN_OBJ_INACTIVE;
      break;
    case ARM_CAN_OBJ_RX_RTR_TX_DATA:
    case ARM_CAN_OBJ_TX_RTR_RX_DATA:
      can_obj_cfg[obj_idx] = ARM_CAN_OBJ_INACTIVE;
      return ARM_DRIVER_ERROR_UNSUPPORTED;
    case ARM_CAN_OBJ_TX:
      if (obj_idx < CAN_RX_OBJ_NUM)  { return ARM_DRIVER_ERROR_PARAMETER; }
      can_obj_cfg[obj_idx] = ARM_CAN_OBJ_TX;
      break;
    case ARM_CAN_OBJ_RX:
      if (obj_idx >= CAN_RX_OBJ_NUM) { return ARM_DRIVER_ERROR_PARAMETER; }
      can_obj_cfg[obj_idx] = ARM_CAN_OBJ_RX;
      break;
    default:
      return ARM_DRIVER_ERROR;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t CAN_MessageSend (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, const uint8_t *data, uint8_t size)
  \brief       Send message on CAN bus.
  \param[in]   obj_idx  Object index
  \param[in]   msg_info Pointer to CAN message information
  \param[in]   data     Pointer to data buffer
  \param[in]   size     Number of data bytes to send
  \return      value >= 0  number of data bytes accepted to send
  \return      value < 0   execution status
*/
static int32_t CAN_MessageSend (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, const uint8_t *data, uint8_t size) {
  uint32_t     data_tx[2];
  uint32_t     tir;

  if ((obj_idx < CAN_RX_OBJ_NUM) || (obj_idx >= CAN_TOT_OBJ_NUM)) { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered == 0U)                                   { return ARM_DRIVER_ERROR;           }
  if (can_obj_cfg[obj_idx] != ARM_CAN_OBJ_TX)                     { return ARM_DRIVER_ERROR;           }

  obj_idx -= CAN_RX_OBJ_NUM;                            // obj_idx origin to 0

  if ((ptr_CAN->sTxMailBox[obj_idx].TIR & CAN_TI0R_TXRQ) != 0U) { return ARM_DRIVER_ERROR_BUSY; }

  if ((msg_info->id & ARM_CAN_ID_IDE_Msk) != 0U) {      // Extended Identifier
    tir = (msg_info->id <<  3) | CAN_TI0R_IDE;
  } else {                                              // Standard Identifier
    tir = (msg_info->id << 21);
  }

  if (size > 8U) { size = 8U; }

  if (msg_info->rtr != 0U) {                            // If send RTR requested
    size = 0U;
    tir |= CAN_TI0R_RTR;

    ptr_CAN->sTxMailBox[obj_idx].TDTR &= ~CAN_TDT0R_DLC;
    ptr_CAN->sTxMailBox[obj_idx].TDTR |=  msg_info->dlc & CAN_TDT0R_DLC;
  } else {
    if (size != 8U) {
      data_tx[0] = 0U;
      data_tx[1] = 0U;
    }
    memcpy((uint8_t *)(&data_tx[0]), data, size);

    ptr_CAN->sTxMailBox[obj_idx].TDLR = data_tx[0];
    ptr_CAN->sTxMailBox[obj_idx].TDHR = data_tx[1];

    ptr_CAN->sTxMailBox[obj_idx].TDTR &= ~CAN_TDT0R_DLC;
    ptr_CAN->sTxMailBox[obj_idx].TDTR |=  size & CAN_TDT0R_DLC;
  }

  ptr_CAN->sTxMailBox[obj_idx].TIR   =  tir | CAN_TI0R_TXRQ;    // Activate transmit

  return ((int32_t)size);
}

/**
  \fn          int32_t CAN_MessageRead (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, uint8_t *data, uint8_t size)
  \brief       Read message received on CAN bus.
  \param[in]   obj_idx  Object index
  \param[out]  msg_info Pointer to read CAN message information
  \param[out]  data     Pointer to data buffer for read data
  \param[in]   size     Maximum number of data bytes to read
  \return      value >= 0  number of data bytes read
  \return      value < 0   execution status
*/
static int32_t CAN_MessageRead (uint32_t obj_idx, ARM_CAN_MSG_INFO *msg_info, uint8_t *data, uint8_t size) {
  uint32_t     data_rx[2];

  if (obj_idx >= CAN_RX_OBJ_NUM)              { return ARM_DRIVER_ERROR_PARAMETER; }
  if (can_driver_powered == 0U)               { return ARM_DRIVER_ERROR;           }
  if (can_obj_cfg[obj_idx] != ARM_CAN_OBJ_RX) { return ARM_DRIVER_ERROR;           }

  if (size > 8U) { size = 8U; }

  if ((ptr_CAN->sFIFOMailBox[obj_idx].RIR & CAN_RI0R_IDE) != 0U) {      // Extended Identifier
    msg_info->id = (0x1FFFFFFFUL & (ptr_CAN->sFIFOMailBox[obj_idx].RIR >>  3)) | ARM_CAN_ID_IDE_Msk;
  } else {                                              // Standard Identifier
    msg_info->id = (    0x07FFUL & (ptr_CAN->sFIFOMailBox[obj_idx].RIR >> 21));
  }

  if ((ptr_CAN->sFIFOMailBox[obj_idx].RIR & CAN_RI0R_RTR) != 0U) {
    msg_info->rtr = 1U;
    size          = 0U;
  } else {
    msg_info->rtr = 0U;
  }

  msg_info->dlc = ptr_CAN->sFIFOMailBox[obj_idx].RDTR & CAN_RDT0R_DLC;

  if (size > 0U) {
    data_rx[0] = ptr_CAN->sFIFOMailBox[obj_idx].RDLR;
    data_rx[1] = ptr_CAN->sFIFOMailBox[obj_idx].RDHR;

    memcpy(data, (uint8_t *)(&data_rx[0]), size);
  }

  if (obj_idx == 1U) {
    ptr_CAN->RF1R = CAN_RF1R_RFOM1;                     // Release FIFO 1 output mailbox
  } else {
    ptr_CAN->RF0R = CAN_RF0R_RFOM0;                     // Release FIFO 0 output mailbox
  }

  return ((int32_t)size);
}

/**
  \fn          int32_t CAN_Control (uint32_t control, uint32_t arg)
  \brief       Control CAN interface.
  \param[in]   control  Operation
                 - ARM_CAN_SET_FD_MODE :            set FD operation mode
                 - ARM_CAN_ABORT_MESSAGE_SEND :     abort sending of CAN message
                 - ARM_CAN_CONTROL_RETRANSMISSION : enable/disable automatic retransmission
                 - ARM_CAN_SET_TRANSCEIVER_DELAY :  set transceiver delay
  \param[in]   arg      Argument of operation
  \return      execution status
*/
static int32_t CAN_Control (uint32_t control, uint32_t arg) {

  if (can_driver_powered == 0U) { return ARM_DRIVER_ERROR; }

  switch (control & ARM_CAN_CONTROL_Msk) {
    case ARM_CAN_ABORT_MESSAGE_SEND:
      if ((arg < CAN_RX_OBJ_NUM) || (arg >= CAN_TOT_OBJ_NUM)) { return ARM_DRIVER_ERROR_PARAMETER; }
      arg -= CAN_RX_OBJ_NUM;
      switch (arg) {
        case 0:
          ptr_CAN->TSR = CAN_TSR_ABRQ0;
          break;
        case 1:
          ptr_CAN->TSR = CAN_TSR_ABRQ1;
          break;
        case 2:
          ptr_CAN->TSR = CAN_TSR_ABRQ2;
          break;
        default:
          return ARM_DRIVER_ERROR_PARAMETER;
      }
      break;
    case ARM_CAN_CONTROL_RETRANSMISSION:
      switch (arg) {
        case 0:
          ptr_CAN->MCR |=  CAN_MCR_NART;
          break;
        case 1:
          ptr_CAN->MCR &= ~CAN_MCR_NART;
          break;
        default:
          return ARM_DRIVER_ERROR_PARAMETER;
      }
      break;
    case ARM_CAN_SET_FD_MODE:
    case ARM_CAN_SET_TRANSCEIVER_DELAY:
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          ARM_CAN_STATUS CAN_GetStatus (void)
  \brief       Get CAN status.
  \return      CAN status ARM_CAN_STATUS
*/
static ARM_CAN_STATUS CAN_GetStatus (void) {
  ARM_CAN_STATUS  can_status;
  uint32_t        esr;

  esr          = ptr_CAN->ESR;
  ptr_CAN->ESR = CAN_ESR_LEC;           // Software set last error code to unused value

  if       ((ptr_CAN->MSR & CAN_MSR_INAK) != 0U)  { can_status.unit_state = ARM_CAN_UNIT_STATE_INACTIVE; }
  else if (((ptr_CAN->BTR & CAN_BTR_LBKM) != 0U) ||
           ((ptr_CAN->BTR & CAN_BTR_SILM) != 0U)) { can_status.unit_state = ARM_CAN_UNIT_STATE_PASSIVE;  }
  else if ((esr & CAN_ESR_BOFF) != 0U)            { can_status.unit_state = ARM_CAN_UNIT_STATE_INACTIVE; }
  else if ((esr & CAN_ESR_EPVF) != 0U)            { can_status.unit_state = ARM_CAN_UNIT_STATE_PASSIVE;  }
  else                                            { can_status.unit_state = ARM_CAN_UNIT_STATE_ACTIVE;   }

  switch ((esr & CAN_ESR_LEC) >> 4) {
    case 0:
      can_status.last_error_code = ARM_CAN_LEC_NO_ERROR;
      break;
    case 1:
      can_status.last_error_code = ARM_CAN_LEC_STUFF_ERROR;
      break;
    case 2:
      can_status.last_error_code = ARM_CAN_LEC_FORM_ERROR;
      break;
    case 3:
      can_status.last_error_code = ARM_CAN_LEC_ACK_ERROR;
      break;
    case 4:
    case 5:
      can_status.last_error_code = ARM_CAN_LEC_BIT_ERROR;
      break;
    case 6:
      can_status.last_error_code = ARM_CAN_LEC_CRC_ERROR;
      break;
    case 7:
      can_status.last_error_code = ARM_CAN_LEC_NO_ERROR;
      break;
  }

  can_status.tx_error_count = (uint8_t)((esr & CAN_ESR_TEC) >> 16);
  can_status.rx_error_count = (uint8_t)((esr & CAN_ESR_REC) >> 24);

  return can_status;
}

/**
  \fn          void CEC_CAN_IRQHandler (void)
  \brief       CAN Interrupt Routine (IRQ).
*/
void CEC_CAN_IRQHandler (void) {
  uint32_t esr, ier;

  if ((CAN->TSR & CAN_TSR_RQCP0) != 0U) {
    if ((CAN->TSR & CAN_TSR_TXOK0) != 0U) {
      if (can_obj_cfg[CAN_RX_OBJ_NUM] == ARM_CAN_OBJ_TX) {
        if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(CAN_RX_OBJ_NUM, ARM_CAN_EVENT_SEND_COMPLETE); }
      }
    }
    CAN->TSR = CAN_TSR_RQCP0;           // Request completed on transmit mailbox 0
  }
  if ((CAN->TSR & CAN_TSR_RQCP1) != 0U) {
    if ((CAN->TSR & CAN_TSR_TXOK1) != 0U) {
      if (can_obj_cfg[CAN_RX_OBJ_NUM+1U] == ARM_CAN_OBJ_TX) {
        if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(CAN_RX_OBJ_NUM+1U, ARM_CAN_EVENT_SEND_COMPLETE); }
      }
    }
    CAN->TSR = CAN_TSR_RQCP1;           // Request completed on transmit mailbox 1
  }
  if ((CAN->TSR & CAN_TSR_RQCP2) != 0U) {
    if ((CAN->TSR & CAN_TSR_TXOK2) != 0U) {
      if (can_obj_cfg[CAN_RX_OBJ_NUM+2U] == ARM_CAN_OBJ_TX) {
        if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(CAN_RX_OBJ_NUM+2U, ARM_CAN_EVENT_SEND_COMPLETE); }
      }
    }
    CAN->TSR = CAN_TSR_RQCP2;           // Request completed on transmit mailbox 2
  }

  if (can_obj_cfg[0] == ARM_CAN_OBJ_RX) {
    if ((CAN->RF0R & CAN_RF0R_FOVR0) != 0U) {
      CAN->RF0R = CAN_RF0R_FOVR0;       // Clear overrun flag
      if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(0U, ARM_CAN_EVENT_RECEIVE | ARM_CAN_EVENT_RECEIVE_OVERRUN); }
    } else if ((CAN->RF0R & CAN_RF0R_FMP0) != 0U) {
      if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(0U, ARM_CAN_EVENT_RECEIVE); }
    }
  } else {
    CAN->RF0R = CAN_RF0R_RFOM0;         // Release FIFO 0 output mailbox if object not enabled for reception
  }

  if (can_obj_cfg[1] == ARM_CAN_OBJ_RX) {
    if ((CAN->RF1R & CAN_RF1R_FOVR1) != 0U) {
      CAN->RF1R = CAN_RF1R_FOVR1;       // Clear overrun flag
      if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(1U, ARM_CAN_EVENT_RECEIVE | ARM_CAN_EVENT_RECEIVE_OVERRUN); }
    } else if ((CAN->RF1R & CAN_RF1R_FMP1) != 0U) {
      if (CAN_SignalObjectEvent != NULL) { CAN_SignalObjectEvent(1U, ARM_CAN_EVENT_RECEIVE); }
    }
  } else {
    CAN->RF1R = CAN_RF1R_RFOM1;         // Release FIFO 1 output mailbox if object not enabled for reception
  }

  // Handle transition from from 'bus off', ' error active' state, or re-enable warning interrupt
  if (CAN_SignalUnitEvent != NULL) {
    esr = CAN->ESR;
    ier = CAN->IER;
    CAN->MSR = CAN_MSR_ERRI;            // Clear error interrupt
    if      (((esr & CAN_ESR_BOFF) != 0U) && ((ier & CAN_IER_BOFIE) != 0U)) { CAN->IER &= ~CAN_IER_BOFIE; CAN_SignalUnitEvent(ARM_CAN_EVENT_UNIT_BUS_OFF); }
    else if (((esr & CAN_ESR_EPVF) != 0U) && ((ier & CAN_IER_EPVIE) != 0U)) { CAN->IER &= ~CAN_IER_EPVIE; CAN_SignalUnitEvent(ARM_CAN_EVENT_UNIT_PASSIVE); }
    else if (((esr & CAN_ESR_EWGF) != 0U) && ((ier & CAN_IER_EWGIE) != 0U)) { CAN->IER &= ~CAN_IER_EWGIE; CAN_SignalUnitEvent(ARM_CAN_EVENT_UNIT_WARNING); }
  }
}


// Exported driver structure
ARM_DRIVER_CAN Driver_CAN1 = {
  CAN_GetVersion,
  CAN_GetCapabilities,
  CAN_Initialize,
  CAN_Uninitialize,
  CAN_PowerControl,
  CAN_GetClock,
  CAN_SetBitrate,
  CAN_SetMode,
  CAN_ObjectGetCapabilities,
  CAN_ObjectSetFilter,
  CAN_ObjectConfigure,
  CAN_MessageSend,
  CAN_MessageRead,
  CAN_Control,
  CAN_GetStatus
};

/*! \endcond */
