/*
 * Copyright (c) 2019, Adrian Chemicz
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

#include "SOMEIP_Layer.h"
#include <string.h>

#if SOME_IP_CRC_EXTENSION
#include <chip.h>
#endif

/*****************************************************************************************
* SOMEIP_DecodeRxMessage() - Decode SOME/IP message and copy result to SomeIpMessage
* structure.
*
* Parameters:
* @someIpRxMessage: pointer to.SomeIpMessage structure which will be initiated by data inside
*  rxBuffer if those data will be correct.
* @externalBufferPointer: Pointer to destination buffer. If this pointer will be different than
*  null then decoded data will be copied to pointed memory area and information about buffer
*  location will be assigned to structure pointed by someIpRxMessage pointer.
* @rxBuffer: pointer to buffer which hold raw SOME/IP message.
* @rxBufferSize: size of data in rxBuffer.
*
* Return: Return true if data stored in rxBuffer is correct otherwise return false.
*****************************************************************************************/
bool SOMEIP_DecodeRxMessage(SomeIpMessage *someIpRxMessage, uint8_t *externalBufferPointer, uint8_t *rxBuffer, uint16_t rxBufferSize)
{
	uint32_t someIpLength = 0;
	uint16_t someIpCrc = 0;
	uint8_t *bufferPointerTmp = someIpRxMessage->payload;

	//check that received frame can be validated(minimal length is fulfiled)
	if(rxBufferSize < SOME_IP_MINIMAL_MESSAGE_SIZE)
	{
		return false;
	}

	//validate length in SOME/IP header
	someIpLength = ((uint32_t)rxBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN] << 24U)
		|((uint32_t)rxBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN + 1] << 16U)
		|((uint32_t)rxBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN + 2] << 8U)
		|(uint32_t)rxBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN + 3];

	if(rxBufferSize != (someIpLength + SOME_IP_HEADER_LENGTH))
	{
		return false;
	}

	//validete protocol version and interface version
	if((rxBuffer[SOME_IP_PROTOCOL_VERSION_FIELD_BEGIN] != SOME_IP_SUPPORTED_PROTOCOL_VERSION)
		|| (rxBuffer[SOME_IP_PROTOCOL_VERSION_FIELD_BEGIN] != SOME_IP_SUPPORTED_INTERFACE_VERSION))
	{
		return false;
	}

	//validate message type
	if((rxBuffer[SOME_IP_MESSAGE_TYPE_FIELD_BEGIN] != SOME_IP_REQUEST_CODE)
		&& (rxBuffer[SOME_IP_MESSAGE_TYPE_FIELD_BEGIN] != SOME_IP_NOTIFICATION_CODE)
		&& (rxBuffer[SOME_IP_MESSAGE_TYPE_FIELD_BEGIN] != SOME_IP_RESPONSE_CODE))
	{
		return false;
	}

#if SOME_IP_CRC_EXTENSION
	//additional CRC protection code part
	if(rxBufferSize >= (SOME_IP_MINIMAL_MESSAGE_SIZE + 2))
	{
		uint16_t someIpCalculatedCrc = 0;

		/* on every request at end will be added two bytes of CRC16 calculated from
	  	  whole packets(this isn't standard SOME/IP behaviour but additional protection) */
		someIpCrc = ((uint16_t)rxBuffer[rxBufferSize - 2] << 8)|rxBuffer[rxBufferSize - 1];

		memset(&rxBuffer[rxBufferSize - 2], 0, 3);

		if(rxBufferSize % 2 != 0)
		{
			someIpCalculatedCrc = Chip_CRC_CRC16((uint16_t*)rxBuffer, (rxBufferSize - 1)/2);
		}
		else
		{
			someIpCalculatedCrc = Chip_CRC_CRC16((uint16_t*)rxBuffer, (rxBufferSize - 2)/2);
		}

		if(someIpCrc != someIpCalculatedCrc)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#endif

	/****************************************************************
	 *	after validation initialize fields of SOME/IP message
	 ****************************************************************/
	someIpRxMessage->serviceId = ((uint16_t)rxBuffer[SOME_IP_SERVICE_ID_FIELD_BEGIN] << 8)
			|rxBuffer[SOME_IP_SERVICE_ID_FIELD_BEGIN + 1];
	someIpRxMessage->methodId = ((uint16_t)rxBuffer[SOME_IP_METHOD_ID_FIELD_BEGIN] << 8)
			|rxBuffer[SOME_IP_METHOD_ID_FIELD_BEGIN + 1];
	someIpRxMessage->messageType = rxBuffer[SOME_IP_MESSAGE_TYPE_FIELD_BEGIN];
	someIpRxMessage->externalPayloadBufferPointer = externalBufferPointer;
#if SOME_IP_CRC_EXTENSION
	someIpRxMessage->payloadSize = rxBufferSize - SOME_IP_MINIMAL_MESSAGE_SIZE - 2;
#else
	someIpRxMessage->payloadSize = rxBufferSize - SOME_IP_MINIMAL_MESSAGE_SIZE;
#endif
	/****************************************************************
	 *	copy payload
	 ****************************************************************/
	if(someIpRxMessage->externalPayloadBufferPointer == NULL)
	{
		if(someIpRxMessage->payloadSize > SOME_IP_MESSAGE_PAYLOAD_SIZE)
		{
			someIpRxMessage->payloadSize = SOME_IP_MESSAGE_PAYLOAD_SIZE;
		}
	}
	else
	{
		bufferPointerTmp = someIpRxMessage->externalPayloadBufferPointer;
	}

	for(uint16_t i = 0; i < someIpRxMessage->payloadSize; i++)
	{
		bufferPointerTmp[i] = rxBuffer[SOME_IP_MINIMAL_MESSAGE_SIZE + i];
	}

	return true;
}

/*****************************************************************************************
* SOMEIP_CodeTxMessage() - Code SOME/IP message and copy to someIpTxMessageBuffer buffer.
*
* Parameters:
* @serviceId: value of service id which will be copied to beggining of header.
* @methodId: value of method id which will be copied to beggining of header.
* @messageType: possible values of message type field was defined in SOME_IP_MESSAGE_TYPE
* 	enum. Request code is SOME_IP_REQUEST_CODE value and response code is SOME_IP_RESPONSE_CODE
* 	value.
* @returnCode: few typical value of response code was defined in SOMEIP_Layer.h file. Example
* 	typical value is SOME_IP_RETURN_CODE_E_OK_VALUE. User can define own return code.
* @someIpTxMessageBuffer: pointer to address where SOME/IP message will be asembled.
* @payloadPointer: pointer to SOME/IP message payload which isn't required because message
* 	can be empty(contain only SOME/IP header).
* @payloadSize: number of bytes which will be copied as SOME/IP message payload. If value will
* 	be equal zero then will be send only SOME/IP header without SOME/IP payload which is allowed.
*
* Return: Return size of generated SOME/IP message.
*****************************************************************************************/
uint16_t SOMEIP_CodeTxMessage(uint16_t serviceId, uint16_t methodId, uint8_t messageType, uint8_t returnCode,
		uint8_t *someIpTxMessageBuffer, uint8_t *payloadPointer, uint16_t payloadSize)
{
	uint16_t crcValue = 0;
	uint16_t someIpLength = 0;

	//clear
	memset(someIpTxMessageBuffer, 0, SOME_IP_MINIMAL_MESSAGE_SIZE);

	//service ID
	someIpTxMessageBuffer[SOME_IP_SERVICE_ID_FIELD_BEGIN] = (uint8_t)serviceId>>8;
	someIpTxMessageBuffer[SOME_IP_SERVICE_ID_FIELD_BEGIN + 1] = (uint8_t)serviceId&0xFF;

	//method ID
	someIpTxMessageBuffer[SOME_IP_METHOD_ID_FIELD_BEGIN] = (uint8_t)methodId>>8;
	someIpTxMessageBuffer[SOME_IP_METHOD_ID_FIELD_BEGIN + 1] = (uint8_t)methodId&0xFF;

	//client ID and sesion ID will be set as zero(value was set during start clearing)

	//set protocol version field, interface version field, type field and return code field
	someIpTxMessageBuffer[SOME_IP_PROTOCOL_VERSION_FIELD_BEGIN] = SOME_IP_SUPPORTED_PROTOCOL_VERSION;
	someIpTxMessageBuffer[SOME_IP_INTERFACE_VERSION_FIELD_BEGIN] = SOME_IP_SUPPORTED_INTERFACE_VERSION;
	someIpTxMessageBuffer[SOME_IP_MESSAGE_TYPE_FIELD_BEGIN] = messageType;
	someIpTxMessageBuffer[SOME_IP_RETURN_CODE_FIELD_BEGIN] = returnCode;

	//copy payload after header
	if((payloadSize > 0) && (payloadPointer!= NULL))
	{
		memcpy(&someIpTxMessageBuffer[SOME_IP_MINIMAL_MESSAGE_SIZE], payloadPointer, payloadSize);
	}

	someIpLength = (SOME_IP_MINIMAL_MESSAGE_SIZE - 8) + payloadSize;

#if SOME_IP_CRC_EXTENSION
	//add CRC field length
	someIpLength += 2;
#endif

	//length
	someIpTxMessageBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN] = (uint8_t)((someIpLength>>24)&0xFF);
	someIpTxMessageBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN + 1] = (uint8_t)((someIpLength>>16)&0xFF);
	someIpTxMessageBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN + 2] = (uint8_t)((someIpLength>>8)&0xFF);
	someIpTxMessageBuffer[SOME_IP_HEADER_LENGTH_FIELD_BEGIN + 3] = (uint8_t)((someIpLength)&0xFF);

#if SOME_IP_CRC_EXTENSION
	memset(&someIpTxMessageBuffer[SOME_IP_MINIMAL_MESSAGE_SIZE + payloadSize], 0, 3);

	if(payloadSize > 0 && payloadSize % 2 != 0)
	{
		crcValue = Chip_CRC_CRC16((uint16_t*)someIpTxMessageBuffer, (SOME_IP_MINIMAL_MESSAGE_SIZE + payloadSize + 1)/2);
	}
	else
	{
		crcValue = Chip_CRC_CRC16((uint16_t*)someIpTxMessageBuffer, (SOME_IP_MINIMAL_MESSAGE_SIZE + payloadSize)/2);
	}

	someIpTxMessageBuffer[SOME_IP_MINIMAL_MESSAGE_SIZE + payloadSize] = crcValue >> 8;
	someIpTxMessageBuffer[SOME_IP_MINIMAL_MESSAGE_SIZE + payloadSize + 1] = crcValue&0xFF;
#endif

	return (someIpLength + 8);
}
