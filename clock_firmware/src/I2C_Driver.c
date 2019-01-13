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

#include "I2C_Driver.h"

static uint16_t SCLL;
static uint16_t SCLH;

I2C_Device_Instance I2C_Device_Instance_Table[NUMBER_OF_I2C_PERIPHERALS];

static uint32_t* I2C_GetBaseAddress(uint8_t portNumber)
{
	uint32_t* basePointer = 0;
	switch(portNumber)
	{
	case 0:
		basePointer = (uint32_t*)LPC_I2C0;
		break;
	default:
		basePointer = 0;
		break;
	}

	return basePointer;
}

static void I2C_SetClockPrescalers(uint8_t portNumber)
{
	/*must be defined by user*/
	switch(portNumber)
	{
	case 0:
#if 0
		SCLL = 0x40;
		SCLH = 0x40;
#else
		SCLL = 0x80;
		SCLH = 0x80;
#endif

		break;
	default:
		SCLL = 0x40;
		SCLH = 0x40;
		break;
	}
}

void I2C_IRQHandler()
{
	I2C_InterruptProccess((uint8_t)0);
}

void I2C_InterruptProccess(uint8_t portNumber)
{
	LPC_I2C_T *I2C_Port = (LPC_I2C_T*)I2C_GetBaseAddress(portNumber);
	uint8_t busStatus = I2C_Port->STAT;

	switch(busStatus)
	{
	case 0x08://call when start was send
		//change device status
		I2C_Device_Instance_Table[portNumber].deviceStatus = I2C_Sending;
		//clear start bit (STAC - START flag Clear bit)
		I2C_Port->CONCLR = (1<<5);

		if(I2C_Device_Instance_Table[portNumber].dataToSend > 0)
		{
			I2C_Device_Instance_Table[portNumber].dataToSendStatus = 0;
			I2C_Port->DAT = I2C_Device_Instance_Table[portNumber].deviceAddress;
		}
		else if(I2C_Device_Instance_Table[portNumber].dataToRead > 0)
		{
			I2C_Device_Instance_Table[portNumber].dataToReadStatus = 0;
			I2C_Port->DAT = (I2C_Device_Instance_Table[portNumber].deviceAddress|1U);
		}

		break;

	case 0x10://call when repeated start was send
		//clear start bit (STAC - START flag Clear bit)
		I2C_Port->CONCLR = (1<<5);

		I2C_Device_Instance_Table[portNumber].dataToReadStatus = 0;
		I2C_Port->DAT = (I2C_Device_Instance_Table[portNumber].deviceAddress|1U);
		break;
	//send data
	case 0x18://call when device address was send and ACK was received
	case 0x20://call when device address was send and ACK wasn't received
	case 0x28://call when data was send and ACK was received
	case 0x30://call when data was send and ACK wasn't received
		//execute when all data was send
		if(I2C_Device_Instance_Table[portNumber].dataToSendStatus
				== I2C_Device_Instance_Table[portNumber].dataToSend)
		{
			//check read buffer
			if(I2C_Device_Instance_Table[portNumber].dataToRead > 0)
			{
				//send start bit (in control set register STA bit - START flag)
				I2C_Port->CONSET = (1<<5);
			}
			else//execute when read data isn't necessary
			{
				//send stop bit(set STO bit - STOP flag.)
				I2C_Port->CONSET = (1<<4);
				I2C_Device_Instance_Table[portNumber].deviceStatus = I2C_WaitingForData;
			}
		}
		else //send data
		{
			I2C_Port->DAT = I2C_Device_Instance_Table[portNumber].sendBuffer[I2C_Device_Instance_Table[portNumber].dataToSendStatus];
			I2C_Device_Instance_Table[portNumber].dataToSendStatus++;
		}
		break;

	case 0x40://call when address of device was send with ACK
	case 0x48://call when address of device was send without ACK
		//check how much read data is needed. If one then clear ACK bit
		if((I2C_Device_Instance_Table[portNumber].dataToRead - I2C_Device_Instance_Table[portNumber].dataToReadStatus) == 1)
			//in control clear register set AAC - Assert acknowledge Clear bit
			I2C_Port->CONCLR = (1<<2);
		else//then more than one then set ACK bit
			//in control set register set AA - Assert acknowledge flag
			I2C_Port->CONSET = (1<<2);
		break;

	case 0x50://data was received. ACK was returned
		//check how much read data is needed. If one then clear ACK bit
		if((I2C_Device_Instance_Table[portNumber].dataToRead - I2C_Device_Instance_Table[portNumber].dataToReadStatus - 1) == 1)
			//in control clear register set AAC - Assert acknowledge Clear bit
			I2C_Port->CONCLR = (1<<2);
		else//then more than one then set ACK bit
			//in control set register set AA - Assert acknowledge flag
			I2C_Port->CONSET = (1<<2);

		I2C_Device_Instance_Table[portNumber].readBuffer[I2C_Device_Instance_Table[portNumber].dataToReadStatus] = I2C_Port->DAT;
		I2C_Device_Instance_Table[portNumber].dataToReadStatus++;
		break;

	case 0x58://data was received and ACK wasn't returned
		I2C_Device_Instance_Table[portNumber].readBuffer[I2C_Device_Instance_Table[portNumber].dataToReadStatus] = I2C_Port->DAT;
		I2C_Device_Instance_Table[portNumber].dataToReadStatus++;
		//in control set register set STOP flag
		I2C_Port->CONSET = (1<<4);

		I2C_Device_Instance_Table[portNumber].deviceStatus = I2C_WaitingForData;
		break;
	}
	//clear interrupt bit (SIC - I2C interrupt Clear bit)
	I2C_Port->CONCLR = (1<<3);
}

void I2C_DriverInit(uint8_t portNumber)
{
	LPC_I2C_T *I2C_Port = (LPC_I2C_T*)I2C_GetBaseAddress(portNumber);
	I2C_SetClockPrescalers(portNumber);

	I2C_Device_Instance_Table[portNumber].deviceStatus = I2C_WaitingForData;
	//turn on options responsible for change GPIO purpose like UART, ADC or SPI
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);

	if(portNumber == 0)
	{
		//I2C reset
		LPC_SYSCON->PRESETCTRL |= 2;
		//connect I2C to AHB bus
		LPC_SYSCON->SYSAHBCLKCTRL |= (1<<5);

		//configure GPIO pin which is used by I2C
		//SCL
		LPC_IOCON->PIO0[4] |= 1;
		//SDA
		LPC_IOCON->PIO0[5] |= 1;
	}

	//clear all bit inside Control Clear register
	I2C_Port->CONCLR = 0x6C;
	//configure presscaller
	I2C_Port->SCLL = SCLL;
	I2C_Port->SCLH = SCLH;

	if(portNumber == 0)
	{
		//enable I2C interrupt
		NVIC_EnableIRQ(I2C0_IRQn);
	}
	//set bit responsible for enable interface(I2EN - I2C interface enable)
	I2C_Port->CONSET = (1<<6);
}

void I2C_SendReadData(uint8_t portNumber, uint8_t deviceAddress, uint8_t *dataToSend, uint8_t sizeOfSendData, uint8_t sizeOfReadData)
{
	LPC_I2C_T *I2C_Port = (LPC_I2C_T*)I2C_GetBaseAddress(portNumber);
	//initialize data
	I2C_Device_Instance_Table[portNumber].deviceAddress = deviceAddress;
	if(dataToSend != 0)
		memcpy(I2C_Device_Instance_Table[portNumber].sendBuffer, dataToSend, sizeOfSendData);

	I2C_Device_Instance_Table[portNumber].dataToSend = sizeOfSendData;
	I2C_Device_Instance_Table[portNumber].dataToRead = sizeOfReadData;

	//change device status after initialize structure
	I2C_Device_Instance_Table[portNumber].deviceStatus = I2C_WaitingForSend;
	//send start bit (in control set register STA bit - START flag)
	I2C_Port->CONSET = (1<<5);
}

I2C_Status I2C_CheckStatus(uint8_t portNumber)
{
	return I2C_Device_Instance_Table[portNumber].deviceStatus;
}

uint8_t* I2C_PointerToInternalReadBuffer(uint8_t portNumber)
{
	return I2C_Device_Instance_Table[portNumber].readBuffer;
}



