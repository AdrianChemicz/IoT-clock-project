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

#ifndef _FRAM_DRIVER_H_
#define _FRAM_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * This module handle FM25W256(256 Kbit FRAM memory). To communicate with module SPI port is used.
 * CS pin isn't provided by HW because SPI is shared with touch screen. FRAM SPI CS pin was set in
 * defines(pin number and port number). In this module also defined section of FRAM which is used
 * by application. SPI initialization don't exist in this module and must be called separately.
 * This module work without any block function like sleep.
 *
 * Simple example code to store and load data from FRAM module:
 *
 *	uint8_t table1[4] = {0x11, 0x22, 0x33, 0x44};
 *	uint8_t table2[4] = {0, 0, 0, 0};
 *
 *  SPI_DriverInit(1, SPI_CLK_IDLE_LOW, SPI_CLK_LEADING);
 *
 *	FRAM_Init(1);
 *
 *	FRAM_Write(500, 4, table1);
 *	for(;!FRAM_Process();){}
 *
 *	FRAM_Read(500, 4, table2);
 *	for(;!FRAM_Process();){}
 *
 */

#define FRAM_PIN_CS_GPIO_PORT 	0
#define FRAM_PIN_CS_GPIO_PIN 	3

#define OPCODE_WREN 	6
#define OPCODE_WRITE 	2
#define OPCODE_READ 	3

//defines which describe block of data stored inside FRAM
#define FRAM_CLOCK_STATE_FIRST_COPY 	0
#define FRAM_CLOCK_STATE_SECOND_COPY 	500
#define FRAM_MEASUREMENT_BACKUP_COPY 	1000
#define FRAM_MEASUREMENT_DATA_BEGIN 	1400

typedef enum OperationType
{
	None = 							0,
	PrepareMemoryWrite = 			1,
	MemoryWrite = 					2,
	PrepareMemoryRead = 			3,
	MemoryRead = 					4,
	FinalizeMemoryTransaction = 	5,
}OperationType;

void FRAM_Init(uint8_t port);
void FRAM_Write(uint16_t dataAddress, uint16_t numOfBytes, uint8_t* writeBufferPointer);
void FRAM_Read(uint16_t dataAddress, uint16_t numOfBytes, uint8_t* readBufferPointer);
bool FRAM_Process(void);

#endif /* _FRAM_DRIVER_H_ */
