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

#include "LCD.h"

static uint8_t portNumber;

static void LCD_Wait(uint16_t time)
{
#if 0 //configuration for 12MHz // for 1 as input wait is equal 1,89ms
	for (volatile uint32_t count = 0; count < (uint32_t)(time<<11);)
#endif
#if 1 //configuration for 48MHz
	for (volatile uint32_t count = 0; count < (uint32_t)(time<<13);)
#endif
	{
		 count++;
	}
}

void LCD_Init(uint8_t port)
{
	//configure GPIO port to use as output
	//Reset
	GPIO_Direction(LCD_GPIO_PORT_RESET, LCD_GPIO_PIN_RESET, GPIO_DIR_OUTPUT);
	//CS
	GPIO_Direction(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, GPIO_DIR_OUTPUT);

	//set High state on Reset and CS
	GPIO_SetState(LCD_GPIO_PORT_RESET, LCD_GPIO_PIN_RESET, true);
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	portNumber = port;
	//configure SPI
	SPI_DriverInit(portNumber, SPI_CLK_IDLE_HIGH, SPI_CLK_TRAILING);

	//Reset LCD
	GPIO_SetState(LCD_GPIO_PORT_RESET, LCD_GPIO_PIN_RESET, false);
	LCD_Wait(1);
	GPIO_SetState(LCD_GPIO_PORT_RESET, LCD_GPIO_PIN_RESET, true);

	//read driver information
	uint16_t driverNumber = LCD_GetRegister(0);
	if (driverNumber == 0x9325 || driverNumber == 0x9328)
	{
		//initialization sequence
		LCD_SetRegister(0x00, 0x0001);// Start oscillator
		LCD_Wait(50);
		LCD_SetRegister(0x01, 0x0000);
		LCD_SetRegister(0x02, 0x0700);
#if ROTATE_SCREEN
		LCD_SetRegister(0x03, 0x1008);
#else
		LCD_SetRegister(0x03, 0x1038);
#endif
		LCD_SetRegister(0x04, 0x0000);
		LCD_SetRegister(0x08, 0x0202);
		LCD_SetRegister(0x09, 0x0000);
		LCD_SetRegister(0x0A, 0x0000);
		LCD_SetRegister(0x0C, 0x0000);
		LCD_SetRegister(0x0D, 0x0000);
		LCD_SetRegister(0x0F, 0x0000);
		LCD_SetRegister(0x10, 0x0000);
		LCD_SetRegister(0x11, 0x0007);
		LCD_SetRegister(0x12, 0x0000);
		LCD_SetRegister(0x13, 0x0000);
		LCD_Wait(200);
		LCD_SetRegister(0x10, 0x1690);
		LCD_SetRegister(0x11, 0x0227);
		LCD_Wait(50);
		LCD_SetRegister(0x12, 0x001A);
		LCD_Wait(50);
		LCD_SetRegister(0x13, 0x1800);
		LCD_SetRegister(0x29, 0x002A);
		LCD_Wait(50);
		LCD_SetRegister(0x30, 0x0000);
		LCD_SetRegister(0x31, 0x0000);
		LCD_SetRegister(0x32, 0x0000);
		LCD_SetRegister(0x35, 0x0206);
		LCD_SetRegister(0x36, 0x0808);
		LCD_SetRegister(0x37, 0x0007);
		LCD_SetRegister(0x38, 0x0201);
		LCD_SetRegister(0x39, 0x0000);
		LCD_SetRegister(0x3C, 0x0000);
		LCD_SetRegister(0x3D, 0x0000);
		LCD_SetRegister(0x20, 0x0000);
		LCD_SetRegister(0x21, 0x0000);
		LCD_SetRegister(0x50, 0x0000);
		LCD_SetRegister(0x51, 0x00EF);
		LCD_SetRegister(0x52, 0x0000);
		LCD_SetRegister(0x53, 0x013F);
		LCD_SetRegister(0x60, 0xA700);
		LCD_SetRegister(0x61, 0x0003);
		LCD_SetRegister(0x6A, 0x0000);
		LCD_SetRegister(0x90, 0X0010);
		LCD_SetRegister(0x92, 0x0000);
		LCD_SetRegister(0x93, 0X0003);
		LCD_SetRegister(0x95, 0X1100);
		LCD_SetRegister(0x97, 0x0000);
		LCD_SetRegister(0x98, 0x0000);
		LCD_SetRegister(0x07, 0x0133);
	}
}

void LCD_SetRegister(uint8_t reg, uint16_t value)
{
	//low state on CS before transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x70);
	SPI_PutByteToTransmitter(portNumber, 0x0);
	SPI_PutByteToTransmitter(portNumber, reg);

	for (; SPI_CheckBusyFlag(portNumber);){}

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x72);
	SPI_PutByteToTransmitter(portNumber, value >> 8);
	SPI_PutByteToTransmitter(portNumber, value & 0xFF);

	for (; SPI_CheckBusyFlag(portNumber);){}
	
	//clear RX buffer
	for (int i = 0; i < 6;i++)
		SPI_ReadByteFromTrasmitter(portNumber);

	//high state on CS after transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);
}

uint16_t LCD_GetRegister(uint8_t reg)
{
	uint16_t value = 0;
	//low state on CS before transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x70);
	SPI_PutByteToTransmitter(portNumber, 0x0);
	SPI_PutByteToTransmitter(portNumber, reg);

	for (; SPI_CheckBusyFlag(portNumber);){}

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x73);
	SPI_PutByteToTransmitter(portNumber, 0);
	SPI_PutByteToTransmitter(portNumber, 0);
	SPI_PutByteToTransmitter(portNumber, 0);

	for (; SPI_CheckBusyFlag(portNumber);){}

	//read data from RX buffer
	SPI_ReadByteFromTrasmitter(portNumber);
	SPI_ReadByteFromTrasmitter(portNumber);
	SPI_ReadByteFromTrasmitter(portNumber);
	SPI_ReadByteFromTrasmitter(portNumber);
	SPI_ReadByteFromTrasmitter(portNumber);

	value = ((uint16_t)SPI_ReadByteFromTrasmitter(portNumber)) << 8;
	value |= SPI_ReadByteFromTrasmitter(portNumber);

	//high state on CS after transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	return value;
}

void LCD_SetPixel(uint16_t xPos, uint16_t yPos, uint16_t color)
{
	LCD_SetRegister(0x20, yPos);
	LCD_SetRegister(0x21, xPos);
	LCD_SetRegister(0x22, color);
}

void LCD_SetPixel_uGui(uint16_t xPos, uint16_t yPos, uint32_t color)
{
	uint16_t convertedColor = (((color >> 19) & 0x1F) << 11) | (((color >> 10) & 0x3F) << 5) | ((color & 0xFF) >> 3);
#if ROTATE_SCREEN
	LCD_SetPixel(MAX_SCREEN_X - xPos, MAX_SCREEN_Y - yPos, convertedColor);
#else
	LCD_SetPixel(xPos, yPos, convertedColor);
#endif
}

void LCD_FillFrame(uint16_t xPos1, uint16_t yPos1, uint16_t xPos2, uint16_t yPos2, uint16_t color)
{
	uint16_t x = (xPos2 - xPos1) + 1;
	uint16_t y = (yPos2 - yPos1) + 1;

	//set border of area which will be filled
	LCD_SetRegister(0x50, yPos1);
	LCD_SetRegister(0x51, yPos2);
	LCD_SetRegister(0x52, xPos1);
	LCD_SetRegister(0x53, xPos2);

	//set begining of area which will be filled
	LCD_SetRegister(0x20, yPos1);
	LCD_SetRegister(0x21, xPos1);

	//low state on CS before transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x70);
	SPI_PutByteToTransmitter(portNumber, 0x0);
	SPI_PutByteToTransmitter(portNumber, 0x22);

	for (; SPI_CheckBusyFlag(portNumber);){}

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	//delay between change GPIO state
	LCD_Wait(1);

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x72);

	for (; SPI_CheckBusyFlag(portNumber);){}

	for (int i = 0; i < (x*y);)
	{
		for (int j = 0; (j < 4) && (i < x*y); i++, j++)
		{
			SPI_PutByteToTransmitter(portNumber, color >> 8);
			SPI_PutByteToTransmitter(portNumber, color & 0xFF);
		}
		for (; SPI_CheckBusyFlag(portNumber);){}
	}

	//clear RX buffer
	for (int i = 0; i < 8; i++)
		SPI_ReadByteFromTrasmitter(portNumber);

	//high state on CS after transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	//restore settings of border - without this part of code function set pixel will not work
	LCD_SetRegister(0x50, 0x0000);
	LCD_SetRegister(0x51, MAX_SCREEN_Y);
	LCD_SetRegister(0x52, 0x0000);
	LCD_SetRegister(0x53, MAX_SCREEN_X);
}

void LCD_FillFrame_uGui(uint16_t xPos1, uint16_t yPos1, uint16_t xPos2, uint16_t yPos2, uint32_t color)
{
	uint16_t convertedColor = (((color >> 19) & 0x1F) << 11) | (((color >> 10) & 0x3F) << 5) | ((color & 0xFF) >> 3);
#if ROTATE_SCREEN
	LCD_FillFrame(MAX_SCREEN_X - xPos2, MAX_SCREEN_Y - yPos2, MAX_SCREEN_X - xPos1, MAX_SCREEN_Y - yPos1, convertedColor);
#else
	LCD_FillFrame(xPos1, yPos1, xPos2, yPos2, convertedColor);
#endif
}

void LCD_StartFillArea_uGui(uint16_t xPos1, uint16_t yPos1, uint16_t xPos2, uint16_t yPos2)
{
	//set border of area which will be filled
#if ROTATE_SCREEN
	LCD_SetRegister(0x50, (MAX_SCREEN_Y - yPos2));
	LCD_SetRegister(0x51, (MAX_SCREEN_Y - yPos1));
	LCD_SetRegister(0x52, (MAX_SCREEN_X - xPos2));
	LCD_SetRegister(0x53, (MAX_SCREEN_X - xPos1));

	//set begining of area which will be filled
	LCD_SetRegister(0x20, (MAX_SCREEN_Y - yPos2));
	LCD_SetRegister(0x21, (MAX_SCREEN_X - xPos2));
#else
	LCD_SetRegister(0x50, yPos1);
	LCD_SetRegister(0x51, yPos2);
	LCD_SetRegister(0x52, xPos1);
	LCD_SetRegister(0x53, xPos2);

	//set begining of area which will be filled
	LCD_SetRegister(0x20, yPos1);
	LCD_SetRegister(0x21, xPos1);
#endif

	//low state on CS before transmision via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x70);
	SPI_PutByteToTransmitter(portNumber, 0x0);
	SPI_PutByteToTransmitter(portNumber, 0x22);

	for (; SPI_CheckBusyFlag(portNumber);){}

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	//delay between change GPIO state
	LCD_Wait(1);

	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, false);

	SPI_PutByteToTransmitter(portNumber, 0x72);

	SPI_PutByteToTransmitter(portNumber, 0x00);
	SPI_PutByteToTransmitter(portNumber, 0x00);

	for (; SPI_CheckBusyFlag(portNumber);){}
}

void LCD_PixelFillArea_uGui(uint32_t color)
{
	uint16_t convertedColor = (((color >> 19) & 0x1F) << 11) | (((color >> 10) & 0x3F) << 5) | ((color & 0xFF) >> 3);

	static uint8_t spiPixelCounter = 0;

	SPI_PutByteToTransmitter(portNumber, convertedColor >> 8);
	SPI_PutByteToTransmitter(portNumber, convertedColor & 0xFF);

	spiPixelCounter++;

	//FIFO length of SPI in LPC microcontroller is equal 8 byte(tested only in 8 bit mode)
	//which mean that 4 pixels can be send
	if(spiPixelCounter >= 4)
	{
		for (; SPI_CheckBusyFlag(portNumber);){}
		spiPixelCounter = 0;
	}
}

void LCD_StopFillArea_uGui(void)
{
	//clear RX buffer
	for (int i = 0; i < 8; i++)
		SPI_ReadByteFromTrasmitter(portNumber);

	//high state on CS after transmission via SPI
	GPIO_SetState(LCD_GPIO_PORT_CS, LCD_GPIO_PIN_CS, true);

	//restore settings of border - without this part of code function set pixel will not work
	LCD_SetRegister(0x50, 0x0000);
	LCD_SetRegister(0x51, MAX_SCREEN_Y);
	LCD_SetRegister(0x52, 0x0000);
	LCD_SetRegister(0x53, MAX_SCREEN_X);

	for (; SPI_CheckBusyFlag(portNumber);){}
}
