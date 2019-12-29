/*

clock_firmware
Copyright (C) 2019 Adrian Chemicz

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>. */

#ifndef _WIFI_INTERACTIONLAYER_H_
#define _WIFI_INTERACTIONLAYER_H_

/*
 * This module is responsible for handle direct communication with ESP_Layer and provide result of
 * communication to higher layer like GUI. Functionality of this module was divide into three
 * separate functions:
 * WIFI_Init - function responsible for initialization ESP8266 like check presence of module,
 *	reset ESP and clear RX buffer before normal operation. This function work only on beggining.
 *	If ESP module will initialize correctly then appropriate flag will be set as true.
 * WIFI_Process - function must be call cyclically after initialization process with correct
 *	result. Information about get connection status, assigned IP address and available APN
 *	is performed inside this function. Inside this part is also performed log to APN if
 *	password was set.
 * WIFI_ProcessRequest - function is only call from WIFI_Process and is only responsible for
 *	process SOME/IP request. If SOME/IP message is correct(correct SOME/IP message header
 *	and CRC if this extension was set) then payload is processed. Inside this function is
 *	performed request validation like chech that service and method is supported and data
 *	for dedicated request is correct.
 * Two last function require pointer to WifiStateType structure as input parameter. Inside this
 * structure is hold data necessary for all operations like cyclically check connection status,
 * get APN list if it is necessary, send TX responses and search FRAM memory to find appropriate
 * data.
 */

#include "ESP_Layer.h"
#include "SOMEIP_Layer.h"
#include "GUI_Clock.h"

#define WIFI_INIT_MULTIPLY_CONNECTION		0
#define WIFI_CREATE_TCP_SERVER				1
#define TCP_SERVER_PORT_NUMBER			 3000
#define MAX_NUMBER_OF_TX_BUFFER				2
#define WIFI_SEND_DATA_REPETITION			3

//supported services and methods
#define SOME_IP_SERVICE_CLOCK_STATUS				1
#define SOME_IP_SERVICE_CLOCK_STATUS_METHOD_SET 	0
#define SOME_IP_SERVICE_CLOCK_STATUS_METHOD_GET 	1
#define SOME_IP_SERVICE_DAY_MEASUREMENT				2

//SOME/IP payload message defines
#define SOME_IP_SERVICE_CLOCK_STATUS_METHOD_SET_PAYLOAD_SIZE	5
#define SOME_IP_SERVICE_CLOCK_STATUS_METHOD_GET_PAYLOAD_SIZE	12
#define SOME_IP_SERVICE_DAY_MEASUREMENT_REQ_PAYLOAD_SIZE		4
#define DAY_MEASUREMENT_HEADER_SIZE								4
#define SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE	40

typedef enum WIFI_STARTUP_PHASES_TYPE
{
	WIFI_STARTUP_WAIT,
	WIFI_SEND_FIRST_REQUEST,
	WIFI_SEND_RESET,
	WIFI_RESET_WAIT,
	WIFI_SEND_ESP_MODE,
	WIFI_SEND_DETECT_REQUEST,
	WIFI_DEVICE_DONT_DETECTED
}WIFI_STARTUP_PHASES;

typedef enum WIFI_REQUESTS_TYPE
{
	WIFI_REQUEST_NONE,
	WIFI_REQUEST_GET_APN,
	WIFI_REQUEST_CONNECT_TO_APN,
	WIFI_REQUEST_DISCONNECT,
	WIFI_REQUEST_GET_IP_ADDRESS,
	WIFI_REQUEST_CONNECTION_STATUS,
	WIFI_REQUEST_ACCEPT_MULLTIPLY_CONNECTION,
	WIFI_REQUEST_CREATE_TCP_SERVER,
	WIFI_REQUEST_SEND_RESPONSE_MESSAGE
}WIFI_REQUESTS;

typedef enum DAY_STRUCTURE_SEARCH_STATUS_TYPE
{
	SEARCH_NOT_REQUESTED,
	SEARCH_REQUESTED,
	SEARCH_PENDING,
	SEARCH_PENDING_READ_READY,
	SEARCH_HEADER_MATCH,
	SEARCH_FINISHED
}DAY_STRUCTURE_SEARCH_STATUS;

typedef struct
{
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
}SomeIpClockStatusSetRequestMethodPayload;

typedef struct
{
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
	uint8_t reserved;
	uint16_t temperatureOutside;
	uint16_t temperatureInside;
	uint16_t temperatureFurnace;
}SomeIpClockStatusGetResponseMethodPayload;

typedef struct
{
	uint8_t source;
	uint8_t day;
	uint8_t month;
	uint8_t year;
}SomeIpDayMeasurmentRequestPayload, DayMeasurementHeader;

typedef struct
{
	uint16_t getApnCounter;
	uint16_t connectToApnCounter;
	uint16_t checkConnectionCounter;
	uint8_t processedRequest;
	uint8_t diconnectRepetitionCounter;
	bool initSocketFlag;
	uint8_t initSocketPhase;

	SocketMessage TxMessageTable[MAX_NUMBER_OF_TX_BUFFER];
	uint8_t sendTxMessageNumber;
	bool sendTxFirstStepWasPerformed;
	uint8_t sendTxErrorCounter;

	//request data
	bool messageIsReceived;
	SomeIpMessage someIpReceivedMessage;

	//temperature per day data
	DayMeasurementHeader SearchedDayMeasurementHeader;	/* This structure contain content copied from SOME/IP request */
	TemperatureSingleDayRecordType TemperatureSingleDayRecordLoadedBuffer; /* Buffer which hold correct last searched day
	measurement if temperatureStructureIsValid is set */
	TemperatureSingleDayRecordType TemperatureSingleDayRecordTmp; /* Structure used as temporary buffer during verification of
	correctness of storing data */
	bool temperatureStructureIsValid;	/* This flag hold information about correctness of TemperatureSingleDayRecordLoadedBuffer
	structure. This flag is used to decide about load process if correct data is stored in TemperatureSingleDayRecordLoadedBuffer
	and header is the same then search isn't necessary */
	bool searchedStructureExist;	// Result of search. if match structure will not find in all FRAM then it will be set as false
	uint8_t searchState;	// This variable hold state used by state machine working on process day measurement request
	bool readFramWasRequested;
	uint16_t searchFramIndex;
}WifiStateType;

void WIFI_Init(void);
void WIFI_Process(WifiStateType* wifiStateStructure);

#endif /* _WIFI_INTERACTIONLAYER_H_ */
