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

#include "UART_Driver.h"

static uint8_t AhbClkDivider;
static uint8_t DivisorLatchMSB;
static uint8_t DivisorLatchLSB;
static uint8_t FractionalDivider;

static uint32_t* UART_GetBaseAddress(uint8_t portNumber)
{
	uint32_t* basePointer = 0;
	switch(portNumber)
	{
	case 0:
		basePointer = (uint32_t*)LPC_USART0_BASE;
		break;
	default:
		basePointer = 0;
		break;
	}

	return basePointer;
}

static void UART_SetClockPrescalers(uint32_t baudrate)
{
	/*must be defined by user  */
	switch(baudrate)
	{
	case 9600:
		AhbClkDivider = 1;
		DivisorLatchMSB = 0;
		DivisorLatchLSB = 1;
		FractionalDivider = 0x10;
		break;
	case 115200:
		AhbClkDivider = 1;
		DivisorLatchMSB = 0;
		DivisorLatchLSB = 1;
		FractionalDivider = 0x10;
		break;
	default:
		break;
	}
}

void UART_DriverInit(uint8_t portNumber, uint32_t baudrate, WORD_LENGTH length, STOP_BITS stopBits, PARITY parity)
{
	LPC_USART0_T *UART_Port = (LPC_USART0_T*)UART_GetBaseAddress(portNumber);

	//turn on options responsible for change GPIO purpose like UART, ADC or SPI
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);

	UART_SetClockPrescalers(baudrate);

	if(portNumber == 0)
	{
		NVIC_DisableIRQ(USART0_IRQn);

		//configure GPIO pin which is used by UART0
		//RXD
		LPC_IOCON->PIO0[18] |= 1;
		//TXD
		LPC_IOCON->PIO0[19] |= 1;

		//connect UART0 to AHB bus
		LPC_SYSCON->SYSAHBCLKCTRL |= (1<<12);
		//UART AHB divider
		LPC_SYSCON->USART0CLKDIV = AhbClkDivider;
	}

	//stop bits, frame length, activate divisor latch
	UART_Port->LCR = ENABLE_DIVISOR_LATCH|(stopBits<<2)|length;

	//configure parity if was enabled
	if(parity != NONE_PARITY)
		UART_Port->LCR = (UART_Port->LCR)|(parity<<4)|(1<<3);

	UART_Port->DLM = DivisorLatchMSB;
	UART_Port->DLL = DivisorLatchLSB;
	UART_Port->FDR = FractionalDivider;

	//deactivate divisor latch
	UART_Port->LCR = (UART_Port->LCR)&(DISABLE_DIVISOR_LATCH);

	//configure FIFO
	UART_Port->FCR = ENABLE_AND_RESET_FIFO;

	//clear RX FIFO
	for(int d=0,tmp=0;d<16;d++)
		tmp=UART_Port->TER;

	NVIC_EnableIRQ(USART0_IRQn);
}

void UART_PutByteToTransmitter(uint8_t portNumber, uint8_t byte)
{
	LPC_USART0_T *UART_Port = (LPC_USART0_T*)UART_GetBaseAddress(portNumber);
	UART_Port->THR = byte;
}

uint8_t UART_ReadByteFromTrasmitter(uint8_t portNumber)
{
	LPC_USART0_T *UART_Port = (LPC_USART0_T*)UART_GetBaseAddress(portNumber);
	return UART_Port->RBR;
}

UART_Status UART_ReturnStatusRegister(uint8_t portNumber)
{
	LPC_USART0_T *UART_Port = (LPC_USART0_T*)UART_GetBaseAddress(portNumber);
	UART_Status status = *((UART_Status*)&(UART_Port->LSR));
	return status;
}
