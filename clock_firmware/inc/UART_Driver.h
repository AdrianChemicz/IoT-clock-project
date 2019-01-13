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

#ifndef _UART_DRIVER_H_
#define _UART_DRIVER_H_

/*
 * This module handle UART of LPC11E6x microcontroler. Most of function directly operate
 * on microcontroler register. Additional logic exist only in UART_DriverInit fuction.
 * UART_DriverInit function contain additional logic like configure UART pins(RX, TX)
 * configure clock of UART and this section should be modify by user(set prescaler
 * registers and fractional divider for appropriate baudrate). UART_DriverInit function
 * should be modify by user microcontroller frequency will be changed or if new baudrate
 * value will be necessary, then new value should be calculated and parameter assign to
 * appropriate field.
 *
 * Simple example code to send and receive data via UART module:
 *
 *  UART_DriverInit(0, 9600, L8_BIT, ONE_BIT, NONE_PARITY);
 *
 *	UART_Status status = UART_ReturnStatusRegister(0);
 *
 *	//process TX data when transmit holding register is empty
 *	if (status.THRE == 1)
 *  {
 * 		UART_PutByteToTransmitter(0, 0x22);
 *		UART_PutByteToTransmitter(0, 0xAA);
 *		UART_PutByteToTransmitter(0, 0xDD);
 *	}
 *
 *  status = UART_ReturnStatusRegister(0);
 *
 *	//check availability RX data inside UART buffer
 * 	for (uint8_t rxBuffer = 0; status.RDR == 1;)
 *	{
 *		rxBuffer = UART_ReadByteFromTrasmitter(0);
 *
 *		//refresh status register
 *		status = UART_ReturnStatusRegister(0);
 *	}
 */

#include <stdint.h>
#include <stdbool.h>

#include "chip.h"

#define UART_BUFFER_SIZE 	16
#define UART_DEVICES 		 5

#ifdef __cplusplus
extern "C" {
#endif

typedef enum WORD_LENGTH
{
	L5_BIT = 	0,
	L6_BIT = 	1,
	L7_BIT = 	2,
	L8_BIT = 	3
}WORD_LENGTH;

typedef enum STOP_BITS
{
	ONE_BIT = 	0,
	TWO_BITS = 	1
}STOP_BITS;

typedef enum PARITY
{
	ODD_PARITY = 	0,
	EVEN_PARITY = 	1,
	MARK_PARITY = 	2,//parity bit is always equal 1
	SPACE_PARITY = 	3,//parity bit is always equal 0
	NONE_PARITY = 	4
}PARITY;

typedef struct
{
	uint32_t RDR : 1; //Receiver Data Ready - set when RX fifo contain unread character
	uint32_t OE : 1; /*Overrun Error - set when RX FIFO can't read character because limit is exceded.
	This error is clear after read Line Status register.*/
	uint32_t PE : 1; /*Parity Error - set when byte on top of RX FIFO cotain this error.
	This error is clear after read Line Status register even if byte on top of RX FIFO contain this error.
	Error occur when parity bit is different than excepted. */
	uint32_t FE : 1; /*Framing Error - error which occur when stop bits of received character is equal
	logic 1. Set when byte on top of RX FIFO cotain this error. This error is clear after read Line Status
	register even if byte on top of RX FIFO contain this error.*/
	uint32_t BI : 1; /*Break Interrupt - error which occur when all bit inside frame(start, data, parity)
	contain zeros(spacing state). Set when byte on top of RX FIFO cotain this error. This error is clear
	after read Line Status register even if byte on top of RX FIFO contain this error. */
	uint32_t THRE : 1; //Transmitter Holding Register Empty - set when TX FIFO is empty
	uint32_t TEMT : 1; //Transmitter Empty - set when UART trasmit shift  register is empty
	uint32_t RXFE : 1; /*Error in RX FIFO - set when one or more byte in RX FIFO contain error like PE, FE, BI.
	Overrun error don't set this bit. This error will clear when broken byte in RX buffer will read. */
	uint32_t Reserved : 24;
}UART_Status;

typedef struct
{
	uint8_t PE : 1;
	uint8_t FE : 1;
	uint8_t BI : 1;
	uint8_t Error : 1;
	uint8_t Reserved : 4;
}ReadByteErrors;

#define ENABLE_DIVISOR_LATCH 		0x80
#define DISABLE_DIVISOR_LATCH 		0x7F
#define ENABLE_AND_RESET_FIFO 		 0x7

void UART_DriverInit(uint8_t portNumber, uint32_t baudrate, WORD_LENGTH length, STOP_BITS stopBits, PARITY parity);
void UART_PutByteToTransmitter(uint8_t portNumber, uint8_t byte);
uint8_t UART_ReadByteFromTrasmitter(uint8_t portNumber);
UART_Status UART_ReturnStatusRegister(uint8_t portNumber);

#ifdef __cplusplus
}
#endif

#endif /* _UART_DRIVER_H_ */
