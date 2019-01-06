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

#include "FRAM_Driver.h"

static uint8_t portNumber;
static OperationType Operation;
static uint8_t* memoryPointer;
static uint16_t numOfDataToCopy;
static uint16_t numOfDataCopied;
static uint16_t dataAddressWrite;

static inline void FRAM_ClearSpiBuffer()
{
	for(uint16_t i = 0;i<SPI_BUFFER_SIZE;i++)
		(void)SPI_ReadByteFromTrasmitter(portNumber);
}

void FRAM_Init(uint8_t port)
{
	portNumber = port;
	Operation = None;
	memoryPointer = 0;

	//configure GPIO
	GPIO_Direction(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, GPIO_DIR_OUTPUT);

	//set pin in high state
	GPIO_SetState(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, true);

	FRAM_ClearSpiBuffer();
}

bool FRAM_Process(void)
{
	bool returnState = false;

	if(SPI_CheckBusyFlag(portNumber))
		return false;

	switch(Operation)
	{
	case PrepareMemoryWrite:
		//set pin in high state after send write enable opcode
		GPIO_SetState(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, true);

		for(uint16_t i = 0;i<16;i++)
			asm("nop");

		//set pin in low state
		GPIO_SetState(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, false);

		//send opcode with write command
		SPI_PutByteToTransmitter(portNumber, OPCODE_WRITE);

		//send address
		SPI_PutByteToTransmitter(portNumber, (uint8_t)(dataAddressWrite>>8));
		SPI_PutByteToTransmitter(portNumber, (uint8_t)(dataAddressWrite & 0xFF));

		Operation = MemoryWrite;
		returnState = false;
		break;

	case MemoryWrite:
		//check finish transaction statement
		if(numOfDataCopied >= numOfDataToCopy)
		{
			Operation = FinalizeMemoryTransaction;
		}

		for(uint16_t i = 0;(i < SPI_BUFFER_SIZE) && (numOfDataCopied < numOfDataToCopy);i++, numOfDataCopied++)
		{
			SPI_PutByteToTransmitter(portNumber, memoryPointer[numOfDataCopied]);
		}
		returnState = false;
		break;

	case PrepareMemoryRead:
		FRAM_ClearSpiBuffer();

		Operation = MemoryRead;
		returnState =  false;
		break;

	case MemoryRead:
		;
		static uint16_t readCounter = 0;

		if(numOfDataCopied == 0)
		{
			readCounter = 0;
			FRAM_ClearSpiBuffer();
		}
		else
		{
			//copy RX data to memory buffer
			for (; SPI_CheckRxFifoNotEmpty(portNumber);readCounter++)
			{
				memoryPointer[readCounter] = SPI_ReadByteFromTrasmitter(portNumber);
			}
		}

		for(uint16_t i = 0;(i < SPI_BUFFER_SIZE) && (numOfDataCopied < numOfDataToCopy);i++, numOfDataCopied++)
			SPI_PutByteToTransmitter(portNumber, 0xFF);

		//check finish transaction statement
		if(readCounter >= numOfDataToCopy)
			Operation = FinalizeMemoryTransaction;
		returnState = false;
		break;

	case FinalizeMemoryTransaction:
		//set pin in high state
		GPIO_SetState(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, true);

		FRAM_ClearSpiBuffer();

		Operation = None;

		returnState = true;
		break;

	case None:
	default:
		break;
	}
	return returnState;
}

void FRAM_Write(uint16_t dataAddress, uint16_t numOfBytes, uint8_t* writeBufferPointer)
{
	//initialize global variable
	numOfDataCopied = 0;
	memoryPointer = writeBufferPointer;
	numOfDataToCopy = numOfBytes;
	Operation = PrepareMemoryWrite;
	dataAddressWrite = dataAddress;

	//set pin in low state
	GPIO_SetState(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, false);

	//send opcode with write enable command
	SPI_PutByteToTransmitter(portNumber, OPCODE_WREN);
}

void FRAM_Read(uint16_t dataAddress, uint16_t numOfBytes, uint8_t* readBufferPointer)
{
	//initialize global variable
	numOfDataCopied = 0;
	memoryPointer = readBufferPointer;
	numOfDataToCopy = numOfBytes;
	Operation = PrepareMemoryRead;

	//set pin in low state
	GPIO_SetState(FRAM_PIN_CS_GPIO_PORT, FRAM_PIN_CS_GPIO_PIN, false);

	//send opcode with write command
	SPI_PutByteToTransmitter(portNumber, OPCODE_READ);

	//send address
	SPI_PutByteToTransmitter(portNumber, (uint8_t)(dataAddress>>8));
	SPI_PutByteToTransmitter(portNumber, (uint8_t)(dataAddress & 0xFF));
}
