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

#ifndef _ESP_LAYER_H_
#define _ESP_LAYER_H_

/*
 * This module handle ESP8266 module connected to microcontroller via UART.
 * API provide by module allow user to:
 * - Connect to APN
 * - Check APN connection status
 * - Create TCP server
 * - Receive and send data to devices connected to TCP server
 * - Monitor link status and gather additional data about connection to TCP server
 * - Check presence of ESP8266 module
 * All function provided by this module work without blocking. Communication
 * with ESP8266 is performed on UART via AT command. Typical schematic of send
 * AT command to ESP8266 via povided API was depicted on below steps:
 *  1.Call function which copy AT command to Tx que if device will not be busy.
 *  If device will be Busy during start execution of function then false will be returned.
 *  2.Call function ESP_Process to copy data from Tx que to UART buffer. This
 * function  must be call cyclically to receive resoponse, process event from
 * module about open links, close links and receive data from module.
 *  3.When function ESP_Process will detect that response was received then
 * deviceStatus will change to RESPONSE_RECEIVED. Status of response can be checked
 * via function ESP_GetRequestState.
 *	4.If deviceStatus is equal RESPONSE_RECEIVED then should be call function which
 * will process response. In this API exist many type of process response functions
 * so called function must be appropriate for request. Description about function which
 * should be use was added to request function description part above function in c file.
 * Many request don't require special process response function because answear is
 * simple(OK\r\n). In this case to those type of function we can use
 * ESP_ProcessGaneralFormatResponse.
 * ESP8266 module not only wait for AT request and send AT response but also send
 * information on event. Those information are open link(example data - 0,CONNECT\r\n ),
 * close link(example data - 0,CLOSED\r\n ) and receive data(example data -
 * \r\n+IPD,0,10:payloadPay ).
 * In communication with ESP8266 module character '\r' mean <CR> or in hex is equal 0x0D,
 * character '\n' mean <LF> or in hex is equal 0x0A.
 * ESP layer provide global structures ApnStructure and RxMessageTable. ApnStructure
 * contain information about available access points and is filled when ESP_SendApnListRequest
 * function will be call and correct response will be processed by ESP_ProcessApnListResponse.
 * RxMessageTable contain information that message received, payload of received message
 * and information about number of link which send message.
 * Known issues:
 * 	-only TCP server is supported
 * 	-necessary change in few places socketNumber to link ID
 *
 * Simple example how to use API:
static void ClockSleep2(uint32_t time)
{
	for (volatile uint32_t count = 0; count < (uint32_t)(time<<10);){count++;}
}

int main(void)
{
	ESP_Init(0, 115200, 10000, 3, 0);
	UART_DisableInterrupts(ESP_GetUartPortNumber());

	ClockSleep2(1000);

	ESP_ClearRxBuffer();

	ESP_SendResetRequest();

	for(;ESP_GetRequestState() == BUSY;)
	{
		ESP_Process();
		ClockSleep2(1);
	}

	if(ESP_ProcessGaneralFormatResponse() == true)
	{
		ClockSleep(1000);
		ESP_ClearRxBuffer();

		ESP_SendDetectDeviceRequest();

		for(;ESP_GetRequestState() == BUSY;)
		{
			ESP_Process();
			ClockSleep2(1);
		}

		//device was reseted and detected
		if(ESP_ProcessGaneralFormatResponse() == true)
		{
			ESP_SendWifiModeRequest(AT_STATION_MODE);

			for(;ESP_GetRequestState() == BUSY;)
			{
				ESP_Process();
				ClockSleep2(1);
			}

			//device was configured correctly to work in station mode
			if(ESP_ProcessGaneralFormatResponse() == false)
			{
				goto endApplication;
			}

			ESP_SendConnectToApnRequest("ZTE-cb82da", "44f436cb19");

			for(;ESP_GetRequestState() == BUSY;)
			{
				ESP_Process();
				ClockSleep2(2);
			}

			//device was connected to APN correctly
			if(ESP_ProcessGaneralFormatResponse() == false)
			{
				goto endApplication;
			}

			ESP_SendAcceptMultipleConnectionRequest();

			for(;ESP_GetRequestState() == BUSY;)
			{
				ESP_Process();
				ClockSleep2(1);
			}

			if(ESP_ProcessGaneralFormatResponse() == false)
			{
				goto endApplication;
			}

			ESP_SendServerCommandRequest(true, 3000);

			for(;ESP_GetRequestState() == BUSY;)
			{
				ESP_Process();
				ClockSleep2(1);
			}

			//ESP was configured to work in expected mode and TCP server run
			if(ESP_ProcessGaneralFormatResponse() == true)
			{
				bool linkStatus = true;
				uint8_t executedAtRequest;
				uint8_t socketId;

				for(uint32_t timer = 1; ; timer++)
				{
					if(ESP_GetRequestState() == READY)
					{
						if(timer%12000 == 0)
						{
							ESP_GetConnectionStatus();
							executedAtRequest = 0;
						}
						else
						{
							//search rx data
							for(uint8_t i = 0; i < MAX_NUMBER_OF_RX_BUFFER; i++)
							{
								if(RxMessageTable[i].lockFlag)
								{
									ESP_SendWriteDataRequest(RxMessageTable[i].socketId, sizeof("TestMessage"));
									socketId = i;
									executedAtRequest = 1;
								}
							}
						}
					}// if(ESP_GetRequestState() == READY)

					ESP_Process();
					ClockSleep2(2);

					if(ESP_GetRequestState() == RESPONSE_RECEIVED)
					{
						switch(executedAtRequest)
						{
						case 0:
							ESP_ProcessConnectionStatus(&linkStatus);
							break;
						case 1:
							if(ESP_ProcessGaneralFormatResponse())
							{
								ESP_Write("TestMessage", sizeof("TestMessage"));
								executedAtRequest = 2;
							}
							else
							{
								RxMessageTable[socketId].lockFlag = false;
							}
							break;
						case 2:
							//send message succesfull
							if(ESP_ProcessGaneralFormatResponse())
							{
								RxMessageTable[socketId].lockFlag = false;
							}
							else//send message fail
							{
								RxMessageTable[socketId].lockFlag = false;
							}
							break;
						}// switch(executedAtRequest)
					}// if(ESP_GetRequestState() == RESPONSE_RECEIVED)
				}// for(uint32_t timer = 1; ; timer++)
			}
		}
	}
endApplication:
	for(;;){}

 */

#include <stdbool.h>
#include <stdint.h>

#define MAX_NUMBER_OF_APN 								10U
#define MAX_NUMBER_OF_SOCKET 							5U
#define INVALID_SOCKET_ID								0xFFU
#define MAX_SIZE_OF_SOCKET_BUFFER 						200U
#define MAX_NUMBER_OF_RX_BUFFER 						5U
#define CONNECT_HEADER_LENGTH 							11U
#define CLOSED_HEADER_LENGTH 							10U
#define TX_RX_BUFFER_SIZE 								400U
#define MINIMAL_CHECKED_RX_SIZE							3U
#define AT_REQ_TIMEOUT_DISABLE 							0U
#define RX_TIMEOUT_DISABLE								0U
#define SSID_STRING_LENGTH 								33U
#define IP_ADDRESS_BYTE_LENGTH 							4U
#define STATUS_STATION_CREATED_TCP_OR_UDP_TRANSMISSION	3U
#define STATUS_STATION_NOT_CONNECTED_TO_APN 			5U
#define TCP_CONNECTION_TYPE								6U
#define UDP_CONNECTION_TYPE								17U

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum DEVICE_STATUS
	{
		READY = 			0,
		BUSY,
		RESPONSE_RECEIVED,
	}DEVICE_STATUS;

	typedef enum ESP_DEVICE_MODE
	{
		AT_STATION_MODE		 	= 1U,
		AT_SOFT_AP_MODE			= 2U,
		AT_SOFT_AP_STATION_MODE	= 3U,
	}ESP_DEVICE_MODE;

	typedef struct{
		uint32_t UART_OVERRUN : 	1;
		uint32_t UART_FRAME_ERROR : 1;
		uint32_t AT_RETURN_ERROR : 	1;
		uint32_t AT_TIMEOUT : 		1;
		uint32_t AT_OVERRUN : 		1;
	}ERROR_CODE;

	typedef struct{
		bool socketIsOpen; /* this flag is refresh when event from ESP will receive and when
		user call ESP_GetConnectionStatus function */
		bool additionalSocketDataIsAvailable;/* this flag inform that below data is available.
		To gather below data is necessary call ESP_GetConnectionStatus function */
		uint8_t connectionType;
		uint8_t remoteIpAddress[IP_ADDRESS_BYTE_LENGTH];
		uint16_t remotePort;
		uint16_t localPort;
		bool tcpConnectionRunAsServer;
	}SocketState;

	typedef struct{
		bool lockFlag;
		uint8_t socketId;
		uint16_t payloadSize;
		uint8_t payload[MAX_SIZE_OF_SOCKET_BUFFER] __attribute__((aligned(32)));
	}SocketMessage;

	typedef struct{
		uint8_t securityLevel;
		int8_t signalPower;
		char ssid[SSID_STRING_LENGTH];//max length plus one
		char bssid[18];
	}ApnInformation;

	typedef struct{
		ApnInformation AppInformationTable[MAX_NUMBER_OF_APN];
		uint8_t NumberOfApn;
	}ApnStructureType;

	typedef struct{
		DEVICE_STATUS deviceStatus;
		ERROR_CODE errorCode;
		uint8_t txBuffer[TX_RX_BUFFER_SIZE];
		uint16_t txSize;
		uint16_t txProgress;
		uint8_t rxBuffer[TX_RX_BUFFER_SIZE];
		uint16_t rxSize;
		uint8_t uartPortNumber;
		//variables used to detect request timeout cause by missing response or lose data
		uint32_t timeoutRequestTreshold;
		uint32_t timeoutRequestCounter;
		//variables used to lock ESP if device will be during receive data
		bool rxDeviceLock;
		uint16_t rxLockCounter;
		uint16_t rxLockTreshold;
		//variables used to clear RX buffer of ESP layer if AT request isn't processed longer time
		uint16_t rxClearCounter;
		uint16_t rxClearThreshold;
		uint16_t lastRxSizeStatus;
	}ESP_Status;

	extern ApnStructureType ApnStructure;
	extern SocketMessage RxMessageTable[MAX_NUMBER_OF_RX_BUFFER];

	void ESP_Init(uint8_t portNum, uint32_t baudrate, uint32_t timeoutRequestTreshold, uint16_t rxLockTreshold, uint16_t rxClearThreshold);
	SocketState ESP_ReturnLinkInformation(uint8_t linkId);
	void ESP_Process(void);
	DEVICE_STATUS ESP_GetRequestState(void);
	bool ESP_SendDetectDeviceRequest(void);
	bool ESP_SendResetRequest(void);
	bool ESP_ProcessGaneralFormatResponse(void);
	bool ESP_SendConnectToApnRequest(const char* ssidName, const char* password);
	bool ESP_SendWifiModeRequest(uint8_t wifiMode);
	bool ESP_SendDisconnectRequest(void);
	bool ESP_SendApnListRequest(void);
	bool ESP_ProcessApnListResponse(void);
	bool ESP_GetAssignedIpAddress(void);
	bool ESP_ProcessGetAssignedIpResponse(uint8_t *pointerToIpAdressTable);
	bool ESP_GetConnectionStatus(void);
	bool ESP_ProcessConnectionStatus(bool *flagPointer);
	bool ESP_SendAcceptMultipleConnectionRequest(void);
	bool ESP_SendServerCommandRequest(bool serverStatus, uint16_t portNumber);
	bool ESP_SendWriteDataRequest(uint8_t socketNumber, uint16_t writeBufferSizeOf);
	bool ESP_Write(uint8_t* bufferPointer, uint16_t bufferSizeOf);
	void ESP_ClearRxBuffer(void);
	uint8_t ESP_GetUartPortNumber(void);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_LAYER_H_ */
