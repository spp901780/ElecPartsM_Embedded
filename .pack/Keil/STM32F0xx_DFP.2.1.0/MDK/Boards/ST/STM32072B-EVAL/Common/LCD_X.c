/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2019  SEGGER Microcontroller GmbH                *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

** emWin V5.50 - Graphical user interface for embedded applications **
All  Intellectual Property rights  in the Software belongs to  SEGGER.
emWin is protected by  international copyright laws.  Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with the following terms:

The software has been licensed to  ARM LIMITED whose registered office
is situated at  110 Fulbourn Road,  Cambridge CB1 9NJ,  England solely
for  the  purposes  of  creating  libraries  for  ARM7, ARM9, Cortex-M
series,  and   Cortex-R4   processor-based  devices,  sublicensed  and
distributed as part of the  MDK-ARM  Professional  under the terms and
conditions  of  the   End  User  License  supplied  with  the  MDK-ARM
Professional. 
Full source code is available at: www.segger.com

We appreciate your understanding and fairness.
----------------------------------------------------------------------
Licensing information
Licensor:                 SEGGER Software GmbH
Licensed to:              ARM Ltd, 110 Fulbourn Road, CB1 9NJ Cambridge, UK
Licensed SEGGER software: emWin
License number:           GUI-00181
License model:            LES-SLA-20007, Agreement, effective since October 1st 2011 
Licensed product:         MDK-ARM Professional
Licensed platform:        ARM7/9, Cortex-M/R4
Licensed number of seats: -
----------------------------------------------------------------------
File        : LCD_X.c
Purpose     : Port routines for STM32F0xx SPI
----------------------------------------------------------------------
*/

#include "GUI.h"

/*********************************************************************
*
*       Hardware configuration
*
**********************************************************************
*/

#include "stm32f0xx.h"                  // Device header
#include "Driver_SPI.h"
#include "stm32f0xx_hal.h"

#include "MX_Device.h"

/* SPI Interface: SPI1

   PINS:
   - CS     = PE6  (GPIO pin)
   - SCK    = PB3  (SPI1_SCK)
   - SDO    = PE14 (SPI1_MISO)
   - SDI    = PE15 (SPI1_MOSI)

   SPI1_MOSI_DIR = PB2 (GPIO pin)
*/
#ifndef MX_SPI1
  #error "::Device:STM32Cube Framework (API): STM32CubeMX: Enable SPI1!"
#endif
#ifndef MX_PE6_Pin
  #error "::Device:STM32Cube Framework (API): STM32CubeMX: Set pin PE6 to GPIO_Output function!"
#endif
#ifndef MX_PB2_Pin
  #error "::Device:STM32Cube Framework (API): STM32CubeMX: Set pin PB2 to GPIO_Output function!"
#endif

/* SPI Driver */
extern ARM_DRIVER_SPI         Driver_SPI1;
#define ptrSPI              (&Driver_SPI1)



/*********************************************************************
*
*       Exported code
*
*********************************************************************
*/
/*********************************************************************
*
*       LCD_X_Init
*
* Purpose:
*   This routine should be called from your application program
*   to set port pins to their initial values
*/
void LCD_X_Init(void) {
  HAL_GPIO_WritePin (GPIOB, GPIO_PIN_2, GPIO_PIN_SET);

  /* Initialize and configure SPI */
  ptrSPI->Initialize  (NULL);
  ptrSPI->PowerControl(ARM_POWER_FULL);
  ptrSPI->Control     (ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_SS_MASTER_UNUSED | ARM_SPI_DATA_BITS(8), 10000000);
}

/*********************************************************************
*
*       LCD_X_ClrCS
*
* Purpose:
*   Sets the chip select pin to low
*/
void LCD_X_ClrCS(void) {
  HAL_GPIO_WritePin (GPIOE, GPIO_PIN_6, GPIO_PIN_RESET);
}

/*********************************************************************
*
*       LCD_X_SetCS
*
* Purpose:
*   Sets the chip select pin to high
*/
void LCD_X_SetCS(void) {
  HAL_GPIO_WritePin (GPIOE, GPIO_PIN_6, GPIO_PIN_SET);
}

/*********************************************************************
*
*       LCD_X_WriteM
*
* Purpose:
*   Writes multiple bytes to controller
*/
void LCD_X_WriteM(U8 * pData, int NumBytes) {
  ptrSPI->Send(pData, NumBytes);
  while (ptrSPI->GetStatus().busy);
}

/*********************************************************************
*
*       LCD_X_ReadM
*
* Purpose:
*   Reads multiple bytes from the controller
*/
void LCD_X_ReadM(U8 * pData, int NumBytes) {
  ptrSPI->Receive(pData, NumBytes);
  while (ptrSPI->GetStatus().busy);
}

/*************************** End of file ****************************/
