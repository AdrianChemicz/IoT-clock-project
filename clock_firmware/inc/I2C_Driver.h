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

#ifndef _I2C_DRIVER_H_
#define _I2C_DRIVER_H_

/*
 * This module handle I2C of LPC11E6x microcontroler and provide layer which process
 * data receive in interupt from interface. Module provide simple interface to store
 * program number of send data and number of read data(function I2C_SendReadData).
 * On next step user check status of transion via function I2C_CheckStatus. When
 * transmision will be finished and sizeOfReadData read data was greater than 0 then
 * pointer to internat read buffer can be returned via I2C_PointerToInternalReadBuffer
 * function. Internal read/write buffer was limited and can be changed by modify
 * READ_WRITE_BUFFER_SIZE define. Prescaler of I2C interface can be changed by modify
 * values in function I2C_SetClockPrescalers.
 *
 * Simple example code to send and receive data via I2C module:
 *
 *  uint8_t sendData[] = { 0x0A };
 *
 *	I2C_DriverInit(0);
 *
 *	I2C_SendReadData(0, 0x3C, sendData, 1, 3);
 *
 *	if (I2C_CheckStatus(0) == I2C_WaitingForData)
 *	{
 *		uint8_t *data = I2C_PointerToInternalReadBuffer(0);
 *
 *
 *	}
 */

#include <stdint.h>
#include <string.h>

#define NUMBER_OF_I2C_PERIPHERALS 1U
#define READ_WRITE_BUFFER_SIZE 32U

#ifdef __cplusplus
extern "C" {
#endif

typedef enum I2C_Status
{
	I2C_WAITING_FOR_DATA,
	I2C_WAITING_FOR_SEND,
	I2C_SENDING,
}I2C_Status;

typedef struct
{
	uint8_t sendBuffer[READ_WRITE_BUFFER_SIZE];
	uint8_t readBuffer[READ_WRITE_BUFFER_SIZE];

	uint8_t dataToSend;
	uint8_t dataToRead;

	uint8_t dataToSendStatus;
	uint8_t dataToReadStatus;

	uint8_t deviceAddress;

	I2C_Status deviceStatus;
}I2C_Device_Instance;

void I2C_DriverInit(uint8_t portNumber);
void I2C_SendReadData(uint8_t portNumber, uint8_t deviceAddress, uint8_t *dataToSend, uint8_t sizeOfSendData, uint8_t sizeOfReadData);
I2C_Status I2C_CheckStatus(uint8_t portNumber);
uint8_t* I2C_PointerToInternalReadBuffer(uint8_t portNumber);

#ifdef __cplusplus
}
#endif

#endif /* _I2C_DRIVER_H_ */
