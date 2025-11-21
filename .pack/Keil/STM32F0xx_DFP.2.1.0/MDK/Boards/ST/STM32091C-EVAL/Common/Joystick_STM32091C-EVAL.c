/*-----------------------------------------------------------------------------
 * Name:    Joystick_STM32091C-EVAL.c
 * Purpose: ADC interface for STM32091C-EVAL board
 * Rev.:    1.0.0
 *----------------------------------------------------------------------------*/

/* Copyright (c) 2013 - 2017 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

/*! \page stm32091b_eval_bsi_joystick Joystick BSI Setup

The BSI Joystick requires:
  - Setup of pins used for Joystick
 
Pinout tab
----------
  1. Click on <b>PA0</b> pin and select <b>GPIO_Input</b> function
  2. Click on <b>PE4</b> pin and select <b>GPIO_Input</b> function
  3. Click on <b>PE5</b> pin and select <b>GPIO_Input</b> function
  4. Click on <b>PE2</b> pin and select <b>GPIO_Input</b> function
  5. Click on <b>PE3/b> pin and select <b>GPIO_Input</b> function

Configuration tab
-----------------
  1. Under System open \b GPIO Configuration:
     - <b>GPIO</b>: Pull-Down on Jostick pins
          Pin Name | Signal on Pin | GPIO outp..|  GPIO mode  | GPIO Pull-up/Pull..| Maximum out |  Fast Mode  | User Label
          :--------|:--------------|:-----------|:------------|:-------------------|:------------|:------------|:----------
          PA0      | n/a           | n/a        | Input mode  | Pull-down..        | n/a         | n/a         
          PE4      | n/a           | n/a        | Input mode  | Pull-down..        | n/a         | n/a         
          PE5      | n/a           | n/a        | Input mode  | Pull-down..        | n/a         | n/a         
          PE2      | n/a           | n/a        | Input mode  | Pull-down..        | n/a         | n/a         
          PE3      | n/a           | n/a        | Input mode  | Pull-down..        | n/a         | n/a         
     Click \b OK to close the GPIO Configuration dialog

*/

#include "Board_Joystick.h"             // ::Board Support:Joystick

#include "stm32f0xx_hal.h"              // Device HAL
#include "MX_Device.h"

typedef struct {
  GPIO_TypeDef *port;
  uint16_t      pin;
  uint16_t      reserved;
} PIN_CONFIG_t;

/* ------------------------------- Configuration ---------------------------- */
#define JOYSTICK_COUNT                 (5)

/* Joystick pins:
   - center: PA0  (GPIO pin)
   - up:     PE4  (GPIO pin)
   - down:   PE5 (GPIO pin)
   - left:   PE2  (GPIO pin)
   - right:  PE3  (GPIO pin)                                                  */

#if ((!defined(MX_PA0_Pin)) || (!defined(MX_PE4_Pin)) ||                      \
     (!defined(MX_PE5_Pin)) || (!defined(MX_PE2_Pin)) || (!defined(MX_PE3_Pin)))
  #error "::Device:STM32Cube Framework (API): STM32CubeMX: Set pins PA0, PE4, PE5 ,PE2 and PE3 to GPIO_Output function!"
#endif

static const PIN_CONFIG_t PIN_CONFIG[JOYSTICK_COUNT] = {
  { MX_PA0_GPIOx, MX_PA0_GPIO_Pin, 0U },        /* Center                     */
  { MX_PE4_GPIOx, MX_PE4_GPIO_Pin, 0U },        /* Up                         */
  { MX_PE5_GPIOx, MX_PE5_GPIO_Pin, 0U },        /* Down                       */
  { MX_PE2_GPIOx, MX_PE2_GPIO_Pin, 0U },        /* Left                       */
  { MX_PE3_GPIOx, MX_PE3_GPIO_Pin, 0U }         /* Right                      */
};
/* -------------------------------------------------------------------------- */

/**
  \fn          int32_t Joystick_Initialize (void)
  \brief       Initialize joystick
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Joystick_Initialize (void) {

  // All initialization code is generated by CubeMX
  return 0;
}

/**
  \fn          int32_t Joystick_Uninitialize (void)
  \brief       De-initialize joystick
  \returns
   - \b  0: function succeeded
   - \b -1: function failed
*/
int32_t Joystick_Uninitialize (void) {

  // All de-initialization code is generated by CubeMX
  return 0;
}

/**
  \fn          uint32_t Joystick_GetState (void)
  \brief       Get joystick state
  \returns     Joystick state
*/
uint32_t Joystick_GetState (void) {
  uint32_t val;

  val  = 0;
  if ((HAL_GPIO_ReadPin (PIN_CONFIG[0].port, PIN_CONFIG[0].pin)) == GPIO_PIN_SET) { val |= JOYSTICK_CENTER; }
  if ((HAL_GPIO_ReadPin (PIN_CONFIG[1].port, PIN_CONFIG[1].pin)) == GPIO_PIN_SET) { val |= JOYSTICK_UP; }
  if ((HAL_GPIO_ReadPin (PIN_CONFIG[2].port, PIN_CONFIG[2].pin)) == GPIO_PIN_SET) { val |= JOYSTICK_DOWN; }
  if ((HAL_GPIO_ReadPin (PIN_CONFIG[3].port, PIN_CONFIG[3].pin)) == GPIO_PIN_SET) { val |= JOYSTICK_LEFT; }
  if ((HAL_GPIO_ReadPin (PIN_CONFIG[4].port, PIN_CONFIG[4].pin)) == GPIO_PIN_SET) { val |= JOYSTICK_RIGHT; }

  return val;
}
