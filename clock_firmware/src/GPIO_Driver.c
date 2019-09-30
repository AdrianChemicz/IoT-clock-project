/*
 * Copyright (c) 2018, 2019, Adrian Chemicz
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
 *
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

#include "GPIO_Driver.h"

#if USE_LPC111x_SERIES
#include "LPC11xx.h"
#endif

#if USE_LPC11E6x_SERIES
#include "chip.h"
#endif

static uint32_t* GPIO_GetBaseAddress(uint8_t portNumber)
{
	uint32_t* basePointer = 0;
#if USE_LPC111x_SERIES
	switch(portNumber)
	{
	case 0:
		basePointer = (uint32_t*)LPC_GPIO0;
		break;
	case 1:
		basePointer = (uint32_t*)LPC_GPIO1;
		break;
	case 2:
		basePointer = (uint32_t*)LPC_GPIO2;
		break;
	case 3:
		basePointer = (uint32_t*)LPC_GPIO3;
		break;
	default:
		basePointer = 0;
		break;
	}
#endif

#if USE_LPC11E6x_SERIES
	return basePointer;
#endif
}

void GPIO_Init(void)
{
	//Defined by user
	//turn on options responsible for change GPIO purpose like UART, ADC or SPI
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);
	//LCD CS - PIO0_12
	LPC_IOCON->PIO0[12] = 1;
	//LCD_Reset - PIO0_11
	LPC_IOCON->PIO0[11] = 1;

	//TP3(Debug purpose) PIO0_16
	GPIO_Direction(0, 16, GPIO_DIR_OUTPUT);
}

void GPIO_Direction(uint8_t port, uint8_t pin, GPIO_DIRECTION dir)
{
#if USE_LPC111x_SERIES
	uint32_t tmpDir = LPC_GPIO0->DIR;
	uint32_t dirMask = ~(1<<pin);
	LPC_GPIO_TypeDef *GPIO_Port = (LPC_GPIO_TypeDef*)GPIO_GetBaseAddress(port);

	GPIO_Port->DIR = (tmpDir & dirMask)|(dir<<pin);
#endif

#if USE_LPC11E6x_SERIES
	uint32_t tmpDir = LPC_GPIO->DIR[port];
	uint32_t dirMask = ~(1<<pin);

	LPC_GPIO->DIR[port] = (tmpDir & dirMask)|(dir<<pin);
#endif
}

void GPIO_SetState(uint8_t port, uint8_t pin, bool state)
{
#if USE_LPC111x_SERIES
	LPC_GPIO_TypeDef *GPIO_Port = (LPC_GPIO_TypeDef*)GPIO_GetBaseAddress(port);
	GPIO_Port->MASKED_ACCESS[1<<pin] = (state<<pin);
#endif

#if USE_LPC11E6x_SERIES
	LPC_GPIO->B[port][pin] = state;
#endif
}

bool GPIO_GetState(uint8_t port, uint8_t pin)
{	
#if USE_LPC111x_SERIES
	LPC_GPIO_TypeDef *GPIO_Port = (LPC_GPIO_TypeDef*)GPIO_GetBaseAddress(port);
	return ((GPIO_Port->MASKED_ACCESS[1<<pin])>>pin);
#endif

#if USE_LPC11E6x_SERIES
	return LPC_GPIO->B[port][pin];
#endif
}
