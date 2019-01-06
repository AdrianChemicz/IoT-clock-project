/*
 * Copyright (c) 2018, Adrian Chemicz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LCD_H_
#define _LCD_H_

/*
 * This module handle LCD with ILI9325 driver. To communicate with LCD only SPI port is used(Change
 * to different interface isn't possible). LCD SPI CS pin isn't provided by HW because because driver
 * expect short interrupt which is force by manipulate GPIO. LCD SPI CS pin was set in defines(pin
 * number and port number). SPI initialization exist in this module and call it separately isn't
 * necessary. LCD driver expect reset pin which number was assign in defines(pin number and port
 * number). This module work in blocking mode so critical code should be call in different thread.
 * Content displayed on LCD can be rotated about 180 degrees by changing define "ROTATE_SCREEN".
 * SPI port isn't shared with different slaves. This module contain special function appropriate for
 * uGui library.
 *
 * Simple example code to draw on LCD was added below:
 *
 *	LCD_Init(0);
 *
 *	LCD_FillFrame(0, 0, 319, 239, 0xFFFF);
 *
 */

#include <stdint.h>
#include "GPIO_Driver.h"
#include "SPI_Driver.h"

#define ROTATE_SCREEN 				 1//Rotate about 180 degrees
#define MAX_SCREEN_X 			0x013F
#define MAX_SCREEN_Y 			0x00EF
#define LCD_GPIO_PORT_RESET 		 0
#define LCD_GPIO_PIN_RESET 			11
#define LCD_GPIO_PORT_CS 			 0
#define LCD_GPIO_PIN_CS 			12

#ifdef __cplusplus
extern "C" {
#endif

	void LCD_Init(uint8_t port);
	void LCD_SetRegister(uint8_t reg, uint16_t value);
	uint16_t LCD_GetRegister(uint8_t reg);
	void LCD_SetPixel(uint16_t xPos, uint16_t yPos, uint16_t color);
	void LCD_SetPixel_uGui(uint16_t xPos, uint16_t yPos, uint32_t color);
	void LCD_FillFrame(uint16_t xPos1, uint16_t yPos1, uint16_t xPos2, uint16_t yPos2, uint16_t color);
	void LCD_FillFrame_uGui(uint16_t xPos1, uint16_t yPos1, uint16_t xPos2, uint16_t yPos2, uint32_t color);
	void LCD_StartFillArea_uGui(uint16_t xPos1, uint16_t yPos1, uint16_t xPos2, uint16_t yPos2);
	void LCD_PixelFillArea_uGui(uint32_t color);
	void LCD_StopFillArea_uGui(void);

#ifdef __cplusplus
}
#endif

#endif /* _LCD_H_ */
