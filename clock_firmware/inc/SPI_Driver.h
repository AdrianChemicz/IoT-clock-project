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

#ifndef _SPI_DRIVER_H_
#define _SPI_DRIVER_H_

/*
 * This module handle SPI of LPC11E6x microcontroler. Most of function directly operate
 * on microcontroler register. Additional logic exist only in SPI_DriverInit fuction.
 * SPI_DriverInit function contain additional logic like configure SPI pins(MISO, MOSI,
 * SCK, CS) configure clock of SPI ports and this section should be configure by
 * user(set prescaler registers). Value for prescaler isn't set as parameter in
 * SPI_DriverInit function beacuse all hardware functions is common for microcontroller
 * and emulated SPI on FTDI chip.
 * Initialization of Hardware Chip select(CS) pin can be disable by set define
 * SPI_HW_CS_ENABLE as 0. After this change in PI_DriverInit function GPIO used by
 * SPI as CS isn't change function and can be used to another purpose.
 *
 * Simple example code to send and receive data via SPI module:
 *
 *  uint8_t spiData[6];
 *
 *  SPI_DriverInit(0, SPI_CLK_IDLE_LOW, SPI_CLK_LEADING);
 *
 *	SPI_PutByteToTransmitter(0, 21);
 *	SPI_PutByteToTransmitter(0, 0);
 *	SPI_PutByteToTransmitter(0, 0);
 *	SPI_PutByteToTransmitter(0, 22);
 *	SPI_PutByteToTransmitter(0, 0);
 *	SPI_PutByteToTransmitter(0, 0);
 *
 *  for (; SPI_CheckBusyFlag(0);){}
 *
 *  spiData[0] = SPI_ReadByteFromTrasmitter(0);
 *  spiData[1] = SPI_ReadByteFromTrasmitter(0);
 *  spiData[2] = SPI_ReadByteFromTrasmitter(0);
 *  spiData[3] = SPI_ReadByteFromTrasmitter(0);
 *  spiData[4] = SPI_ReadByteFromTrasmitter(0);
 *  spiData[5] = SPI_ReadByteFromTrasmitter(0);
 */

#include <stdint.h>
#include <stdbool.h>


#define SPI_BUFFER_SIZE 		8
#define STANDARD_FRAME_LENGTH 	7U
#define SPI_ENABLE 				2U
#define SPI_HW_CS_ENABLE		0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	uint32_t TFE : 1; /* Transmit FIFO Empty */
	uint32_t TNF : 1; /* Transmit FIFO Not Full */
	uint32_t RNE : 1; /* Receive FIFO Not Empty */
	uint32_t RFF : 1; /* Receive FIFO Full */
	uint32_t BSY : 1; /* Busy flag */
	uint32_t Reserved : 27;
}SPI_Status;

typedef enum SPI_CLK_POL
{
	SPI_CLK_IDLE_LOW = 		0,
	SPI_CLK_IDLE_HIGH = 	1
}SPI_CLK_POL;

typedef enum SPI_CLK_PHASE
{
	SPI_CLK_LEADING = 		0,
	SPI_CLK_TRAILING = 		1
}SPI_CLK_PHASE;

void SPI_DriverInit(uint8_t portNumber, SPI_CLK_POL polarity, SPI_CLK_PHASE phase);
void SPI_PutByteToTransmitter(uint8_t portNumber, uint8_t byte);
uint8_t SPI_ReadByteFromTrasmitter(uint8_t portNumber);

bool SPI_CheckTxFifoEmpty(uint8_t portNumber);
bool SPI_CheckTxFifoNotFull(uint8_t portNumber);
bool SPI_CheckRxFifoNotEmpty(uint8_t portNumber);
bool SPI_CheckRxFifoFull(uint8_t portNumber);
bool SPI_CheckBusyFlag(uint8_t portNumber);

SPI_Status SPI_ReturnStatusRegister(uint8_t portNumber);

#ifdef __cplusplus
}
#endif

#endif  /* _SPI_DRIVER_H_ */
