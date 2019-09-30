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

#ifndef _SOMEIP_LAYER_H_
#define _SOMEIP_LAYER_H_

/*
 * This module is responsible for code and decode SOME/IP messages. API in module provide
 * two functions one for code message and one for decode. User of this module in higher
 * layer must decide about response.
 * SomeIpMessage structure is used as container to SOME/IP data gathered by decode receive
 * data. Structure have hardcoded max data in payload field. This limit can be change by
 * modify SOME_IP_MESSAGE_PAYLOAD_SIZE define.
 * Module contain extension to message which add CRC16 to protect payload because is possible
 * that message received/send  via UART or other simple interface will contain errors.
 * Extension can be disabled by set SOME_IP_CRC_EXTENSION define as 0.
 * In define was added few typical error code which can be use by user in higher layer
 * those errors are unknown service, unknown method and malformed message.
 *
 * Header of SOME/IP message:
 *	Service ID:          (2 bytes length)
 *	Method ID:           (2 bytes length)
 *	Length:              (4 bytes length)
 *	Client ID:           (2 bytes length)
 *	Session ID:          (2 bytes length)
 *	Protocol Version:    (1 byte length)
 *	Interface Version:   (1 byte length)
 *	Message Type:        (1 byte length)
 *	Return Code:         (1 byte length)
 *
 * Length field value is calculated from content below length field so for SOME/IP message
 * which not include payload length is equal 8 bytes.
 *
 * Simple example how to code and decode message via provided API:
 *
#define SOME_IP_SERVICE_CLOCK_STATUS				1
#define SOME_IP_SERVICE_CLOCK_STATUS_METHOD_SET 	0
#define SOME_IP_SERVICE_CLOCK_STATUS_METHOD_GET 	1
#define SOME_IP_SERVICE_DAY_MEASUREMENT				2

	uint8_t messageBuffer[40];
	uint16_t codedMessageSize = 0;
	bool decodeFlagState = false;
	volatile SomeIpMessage someIpMessageDecoded;

	codedMessageSize = SOMEIP_CodeTxMessage(SOME_IP_SERVICE_CLOCK_STATUS, SOME_IP_SERVICE_CLOCK_STATUS_METHOD_GET,
		SOME_IP_REQUEST_CODE, SOME_IP_RETURN_CODE_E_OK_VALUE, messageBuffer, NULL, 0);

	decodeFlagState = SOMEIP_DecodeRxMessage(&someIpMessageDecoded, NULL, messageBuffer, codedMessageSize);
 */

#include <stdint.h>
#include <stdbool.h>

#define SOME_IP_CRC_EXTENSION					1
#define SOME_IP_CRC_SIZE						2
#define SOME_IP_MESSAGE_PAYLOAD_SIZE 			10
#define SOME_IP_MINIMAL_MESSAGE_SIZE 			16 //message size without some/ip additional payload(arguments)
#define SOME_IP_SERVICE_ID_FIELD_BEGIN 			0
#define SOME_IP_METHOD_ID_FIELD_BEGIN			2
#define SOME_IP_HEADER_LENGTH 					8
#define SOME_IP_HEADER_LENGTH_FIELD_BEGIN 		4
#define SOME_IP_PROTOCOL_VERSION_FIELD_BEGIN 	12
#define SOME_IP_INTERFACE_VERSION_FIELD_BEGIN 	13
#define SOME_IP_SUPPORTED_PROTOCOL_VERSION 		1
#define SOME_IP_SUPPORTED_INTERFACE_VERSION		1
#define SOME_IP_MESSAGE_TYPE_FIELD_BEGIN		14
#define SOME_IP_RETURN_CODE_FIELD_BEGIN			15

//typical return code. Other can be define by user
#define SOME_IP_RETURN_CODE_E_OK_VALUE			0
#define SOME_IP_RETURN_CODE_E_UNKNOWN_SERVICE	2
#define SOME_IP_RETURN_CODE_E_UNKNOWN_METHOD	3
#define SOME_IP_RETURN_CODE_E_MALFORMED_MESSAGE 9 //raise when deserialization error occur

typedef enum SOME_IP_MESSAGE_TYPE_T
{
	SOME_IP_REQUEST_CODE = 				0x00,
	SOME_IP_REQUEST_NO_RETURN_CODE = 	0x01,
	SOME_IP_NOTIFICATION_CODE = 		0x02,
	SOME_IP_RESPONSE_CODE = 			0x80,
	SOME_IP_ERROR_CODE =				0x81
}SOME_IP_MESSAGE_TYPE;

typedef struct
{
	uint16_t serviceId;
	uint16_t methodId;
	uint8_t messageType;
	uint16_t payloadSize;
	uint8_t payload[SOME_IP_MESSAGE_PAYLOAD_SIZE];
	uint8_t *externalPayloadBufferPointer;
	uint8_t socketId; //ESP extension
}SomeIpMessage;

bool SOMEIP_DecodeRxMessage(SomeIpMessage *someIpRxMessage, uint8_t *externalBufferPointer, uint8_t *rxBuffer, uint16_t rxBufferSize);
uint16_t SOMEIP_CodeTxMessage(uint16_t serviceId, uint16_t methodId, uint8_t messageType, uint8_t returnCode,
		uint8_t *someIpTxMessageBuffer, uint8_t *payloadPointer, uint16_t payloadSize);

#endif  /* _SOMEIP_LAYER_H_ */
