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

#include "ESP_Layer.h"
#include "UART_Driver.h"
#include <string.h>

static ESP_Status ESP_DeviceStatus;
static SocketState SocketStateTable[MAX_NUMBER_OF_SOCKET];

SocketMessage RxMessageTable[MAX_NUMBER_OF_RX_BUFFER];
ApnStructureType ApnStructure;

/*****************************************************************************************
* strCatWithSpecialCharacters() - concatenates two strings. s2 string is append on s1
* string. If s2 string will contain special characters like space or backslash then
* to s1 string will be added additional backslash character. This format is require by
* ESP AT commands.
*
* Parameters:
* @s1: destination string
* @s1: source string which will be append
*
*****************************************************************************************/
static void strCatWithSpecialCharacters(char* s1, char* s2)
{
	for(uint16_t i = 0, len = strlen(s1); i < strlen(s2); i++)
	{
		if((s2[i] == '\\') || (s2[i] == '"'))
		{
			s1[len++] = '\\';
			s1[len++] = s2[i];
			s1[len] = '\0';
		}
		else
		{
			s1[len++] = s2[i];
			s1[len] = '\0';
		}
	}
}

/*****************************************************************************************
* ESP_DataSendInit() - function used to initialize ESP layer before copy AT command
* request to internal buffer. This function is always call from all AT request function.
*
* Parameters:
* @dataSize: size of all AT command request. In few cases must be necessary resize of
* ESP_DeviceStatus.txSize which is initialize by this parameter. This situation can happen
* when AT request isn't constant.
*
*****************************************************************************************/
static void ESP_DataSendInit(uint16_t dataSize)
{
	ESP_DeviceStatus.deviceStatus = BUSY;
	memset(&ESP_DeviceStatus.errorCode, 0, sizeof(ERROR_CODE));
	ESP_DeviceStatus.txSize = dataSize;
	ESP_DeviceStatus.txProgress = 0;
	ESP_DeviceStatus.rxSize = 0;
	ESP_DeviceStatus.timeoutRequestCounter = 0;
	ESP_DeviceStatus.rxDeviceLock = false;
	ESP_DeviceStatus.rxLockCounter = 0;

	//init begin of buffer
	memcpy(ESP_DeviceStatus.txBuffer, "AT+\0", 4);
}

/*****************************************************************************************
* ESP_CopyToInternalBuffer() - called from ESP_Process function when complete asynchronous
* +IPD message(received data) will be completely received. Received data is copied to first
* free Rx buffer in RxMessageTable.
*
* Parameters:
* @socketNumber: number of socket(link) which is idetifier of opened connection. This
*  identifier can be used to recognize which device and opened socket send data.
*  Using function ESP_ReturnLinkInformation is possible to gather more information
*  about socket.
* @bufferPointer: pointer to first element of data which will be copied to RX buffer.
* @bufferSizeOf: size of data which will be copied to RX buffer.
*
*****************************************************************************************/
static void ESP_CopyToInternalBuffer(uint8_t socketNumber, uint8_t* bufferPointer, uint16_t bufferSizeOf)
{
	for(int i = 0; i < MAX_NUMBER_OF_RX_BUFFER; i++)
	{
		if(RxMessageTable[i].lockFlag == false)
		{
			RxMessageTable[i].socketId = socketNumber;
			RxMessageTable[i].payloadSize = bufferSizeOf;
			memcpy(RxMessageTable[i].payload, bufferPointer, bufferSizeOf);
			RxMessageTable[i].lockFlag = true;
			break;
		}
	}
}

/*****************************************************************************************
* ESP_DeleteDataFromRxBuffer() - function used to delete part of data from
* ESP_DeviceStatus.rxBuffer. Data can be present in center of rx buffer then memory will be
* shifted. Function called from ESP_Process when +IPD message(received data) will be
* completely received to delete this message from rx buffer.
*
* Parameters:
* @rxPointer: pointer to beggining place in ESP_DeviceStatus.rxBuffer which will be cleared.
* @size: size of data which will be cleared in ESP_DeviceStatus.rxBuffer.
*
*****************************************************************************************/
static void ESP_DeleteDataFromRxBuffer(uint8_t* rxPointer, uint16_t size)
{
	uint32_t positionInsideRxBuffer = (rxPointer + size) - ESP_DeviceStatus.rxBuffer;
	memmove(rxPointer, (rxPointer + size), ESP_DeviceStatus.rxSize - (positionInsideRxBuffer));

	//add null termination character and decrease rxSize
	ESP_DeviceStatus.rxSize -= size;
	ESP_DeviceStatus.rxBuffer[ESP_DeviceStatus.rxSize] = '\0';
}

/*****************************************************************************************
* ESP_ParseIpAddress() - convert IP address from string to byte table. Function expect data
* in as example '"192.168.1.3"' where first character of string is quotation mark(").
*
* Parameters:
* @stringPointer: pointer to string with IP address which will be parsed
* @ipAddressTable: pointer to table where raw number with IP address will be stored.
*
* Return: Return true if string with IP address is in corrected format.
*****************************************************************************************/
static bool ESP_ParseIpAddress(uint8_t* stringPointer, uint8_t* ipAddressTable)
{
	for(int i = 0; i < IP_ADDRESS_BYTE_LENGTH; i++)
	{
		ipAddressTable[i] = atoi(stringPointer + 1);
		stringPointer++;

		if(i != (IP_ADDRESS_BYTE_LENGTH - 1))
		{
			stringPointer = strstr(stringPointer, ".");

			if(stringPointer == NULL)
			{
				return false;
			}
		}
	}/* for(int i = 0; i < IP_ADDRESS_BYTE_LENGTH; i++) */
	return true;
}

/*****************************************************************************************
* ESP_ReturnLinkInformation() - return socket activity information assigned to linkId.
* Those information are that socket is open, IP address of connected device, source and
* destination port number. Basic information like 'socketIsOpen' flag is updated by
* ESP_Process function but other fields of SocketState flags will be updated when
* ESP_GetConnectionStatus function will call and apropriate request process function will
* be executed without errors.
*
* Parameters:
* @linkId: index used to get information from SocketStateTable.
*
* Return: SocketState structure with information that socket(link) is active or if more
* information is available(flag additionalSocketDataIsAvailable is set) then is possible
* to get information like IP address of connected device.
*****************************************************************************************/
SocketState ESP_ReturnLinkInformation(uint8_t linkId)
{
	if(linkId > (MAX_NUMBER_OF_SOCKET - 1))
	{
		SocketState socketStateTmp;
		socketStateTmp.additionalSocketDataIsAvailable = false;
		socketStateTmp.socketIsOpen = false;

		return socketStateTmp;
	}
	else
	{
		return SocketStateTable[linkId];
	}
}

/*****************************************************************************************
* ESP_Init() - function initialize ESP layer by configuring basic parameters like baudrate
* and timeouts. Timeout counters is increment when appropriate condition will be fulfiled
* and function ESP_Process will be call. When timeout values will be set should be analyze
* time between call ESP_Process and delay between AT request and AT response. In one test
* case time between request and response consume almost 10 seconds. Set timeout values as
* zero will cause that timeout in set functionality will be disabled.
*
* Parameters:
* @portNum: UART port number used by ESP layer.
* @baudrate: baudrate of UART port used by ESP layer.
* @timeoutRequestTreshold: value used as wait limit for AT response.
* @rxLockTreshold: if in RX buffer occur receive event(+IPD:) then device will set as
*	BUSY. This value decide how long device should be blocked. This treshold prevent by
*	situation when user start send AT request command during ESP8266 transmision.
* @rxClearThreshold: number of cycles when RX buffer will be clear. To increment counter
*	which will compare with this thresholdmust be pass two condition. First condition is
*	device is on READY state. Second condition data in RX buffer is greater then zero.
*	If ESP layer is very often used to send request then this threshold cam be set as
*	zero.
"
*****************************************************************************************/
void ESP_Init(uint8_t portNum, uint32_t baudrate, uint32_t timeoutRequestTreshold, uint16_t rxLockTreshold, uint16_t rxClearThreshold)
{
	UART_DriverInit(portNum, baudrate, L8_BIT, ONE_BIT, NONE_PARITY);

	//initialize field inside structure
	ESP_DeviceStatus.uartPortNumber = portNum;
	ESP_DeviceStatus.deviceStatus = READY;
	memset(&ESP_DeviceStatus.errorCode, 0, sizeof(ERROR_CODE));
	ESP_DeviceStatus.rxLockTreshold = rxLockTreshold;
	ESP_DeviceStatus.rxLockCounter = 0;
	ESP_DeviceStatus.timeoutRequestTreshold = timeoutRequestTreshold;
	ESP_DeviceStatus.timeoutRequestCounter = 0;
	ESP_DeviceStatus.rxClearThreshold = rxClearThreshold;
	ESP_DeviceStatus.rxClearCounter = 0;
	ESP_DeviceStatus.lastRxSizeStatus = 0;
	ESP_DeviceStatus.txProgress = 0;
	ESP_DeviceStatus.txSize = 0;
	ESP_DeviceStatus.rxSize = 0;
}

/*****************************************************************************************
* ESP_Process() - function responsible for cyclicaly check data inside UART buffers,
* process received data from ESP module and copy request to UART TX buffer.
* This function must be call cyclically inside loop.
* Function can be also call in interrupt from UART but in this case must be prevented
* situation when ESP_Process is executed and in UART interrupt this function also will call.
* Situation like this is low risk but can happen.
* To prevent it call ESP_Process function like below(only if UART interrupt is used):
*	UART_DisableInterrupts(ESP_GetUartPortNumber());
*	ESP_Process();
*	UART_EnableInterrupts(ESP_GetUartPortNumber());
*
* Function during call move data from tx ESP layer buffer to UART register,
* move data from UART rx register to rx ESP layer buffer, react on event like
* open link(example data - 0,CONNECT\r\n ), close link(example data - 0,CLOSED\r\n )
* and receive data(example data - \r\n+IPD,0,10:payloadPay ).
*
*****************************************************************************************/
void ESP_Process(void)
{
	UART_Status status = UART_ReturnStatusRegister(ESP_DeviceStatus.uartPortNumber);

	//process TX data when transmit holding register is empty
	if(status.THRE == 1)
	{
		//copy max 16 byte or less if part of request is less
		for(uint32_t i = 0; i < UART_BUFFER_SIZE && ESP_DeviceStatus.txProgress < ESP_DeviceStatus.txSize; i++)
		{
			UART_PutByteToTransmitter(ESP_DeviceStatus.uartPortNumber, 
				ESP_DeviceStatus.txBuffer[ESP_DeviceStatus.txProgress]);

			ESP_DeviceStatus.txProgress++;

			//clear timeuot counter
			ESP_DeviceStatus.timeoutRequestCounter = 0;
		}
	}

	//check errors flags inside status register
	if(status.OE == 1)
		ESP_DeviceStatus.errorCode.UART_OVERRUN = 1;

	if(status.RXFE == 1)
		ESP_DeviceStatus.errorCode.UART_FRAME_ERROR;

	//process RX data
	//check availability RX data inside UART buffer
	for(; status.RDR == 1;)
	{
		ESP_DeviceStatus.rxBuffer[ESP_DeviceStatus.rxSize] = UART_ReadByteFromTrasmitter(ESP_DeviceStatus.uartPortNumber);
		ESP_DeviceStatus.rxSize++;

		//add termination character to end of string
		ESP_DeviceStatus.rxBuffer[ESP_DeviceStatus.rxSize] = '\0';
		
		//refresh status register
		status = UART_ReturnStatusRegister(ESP_DeviceStatus.uartPortNumber);

		//clear timeuot counters
		ESP_DeviceStatus.timeoutRequestCounter = 0;

		if(ESP_DeviceStatus.rxDeviceLock == true)
			ESP_DeviceStatus.rxLockCounter = 0;

		ESP_DeviceStatus.rxClearCounter = 0;
	}/* for(; status.RDR == 1;) */

	//detect end of frame (search OK<CR><NL> or ERROR<CR><NL> )
	if(ESP_DeviceStatus.rxSize > MINIMAL_CHECKED_RX_SIZE)
	{
		char* occurencePointer = strstr(ESP_DeviceStatus.rxBuffer, "OK\r\n");
		if(occurencePointer != NULL)
		{
			ESP_DeviceStatus.deviceStatus = RESPONSE_RECEIVED;
			memcpy(occurencePointer, "----", 4);
		}
		occurencePointer = strstr(ESP_DeviceStatus.rxBuffer, "ERROR\r\n");
		if(occurencePointer != NULL)
		{
			ESP_DeviceStatus.errorCode.AT_RETURN_ERROR = 1;
			ESP_DeviceStatus.deviceStatus = RESPONSE_RECEIVED;
			memcpy(occurencePointer, "-------", 7);
		}
	}/* if (ESP_DeviceStatus.rxSize > MINIMAL_CHECKED_RX_SIZE) */

	//search information from ESP module about socket connection state (,CONNECT\r\n and ,CLOSED\r\n)
	if(ESP_DeviceStatus.rxSize > MINIMAL_CHECKED_RX_SIZE)
	{
		uint8_t socketNumber = INVALID_SOCKET_ID;

		uint8_t* occurencePointerConnect = strstr(ESP_DeviceStatus.rxBuffer+1, ",CONNECT\r\n");
		uint8_t* occurencePointerIpd = strstr(ESP_DeviceStatus.rxBuffer, "+IPD");

		if(occurencePointerConnect != NULL && ((occurencePointerIpd == NULL) || (occurencePointerConnect < occurencePointerIpd)))
		{
			socketNumber = atoi(occurencePointerConnect - 1);

			if(socketNumber > (MAX_NUMBER_OF_SOCKET - 1))
			{
				SocketStateTable[socketNumber].socketIsOpen = true;
			}

			//shift memory2
			ESP_DeleteDataFromRxBuffer((occurencePointerConnect - 1), CONNECT_HEADER_LENGTH);
		}
		
		uint8_t* occurencePointerClosed = strstr(ESP_DeviceStatus.rxBuffer+1, ",CLOSED\r\n");

		if(occurencePointerClosed != NULL && ((occurencePointerIpd == NULL) || (occurencePointerClosed < occurencePointerIpd)))
		{
			if(socketNumber > (MAX_NUMBER_OF_SOCKET - 1))
			{
				//check that in one rxBuffer don't exist two event(connect and closed)
				if(socketNumber == atoi(occurencePointerClosed - 1))
				{
					//if exist both event the last state is more important
					if(occurencePointerConnect > occurencePointerClosed)
					{
						SocketStateTable[socketNumber].socketIsOpen = true;
					}
					else
					{
						SocketStateTable[socketNumber].socketIsOpen = false;
						SocketStateTable[socketNumber].additionalSocketDataIsAvailable = false;
					}
				}
				else//if no only assign state to table
				{
					socketNumber = atoi(occurencePointerClosed - 1);
					SocketStateTable[socketNumber].socketIsOpen = false;
				}
			}
			//shift memory
			ESP_DeleteDataFromRxBuffer((occurencePointerClosed - 1), CLOSED_HEADER_LENGTH);
		}/* if(occurencePointerClosed != NULL && ((occurencePointerIpd == NULL) || (occurencePointerClosed < occurencePointerIpd))) */
	}/* if (ESP_DeviceStatus.rxSize > MINIMAL_CHECKED_RX_SIZE) */

	//detect receive message. Example message +IPD,0,5:abcde
	if(ESP_DeviceStatus.rxSize > MINIMAL_CHECKED_RX_SIZE)
	{
		uint8_t* rxFrameBegining = strstr(ESP_DeviceStatus.rxBuffer, "+IPD,");
		uint8_t* occurencePointer = rxFrameBegining;

		//check that appriapriate phrase exist and header is complete
		if((occurencePointer != NULL) && (strstr(rxFrameBegining, ":") != NULL))
		{
			uint32_t headerLength = 0;
			uint16_t numberOfReceivedBytes = atoi(occurencePointer + 7);
			uint8_t numberOfSocket = atoi(occurencePointer + 5);

			//calculate payload begining of received message
			occurencePointer = strstr(occurencePointer, ":");
			headerLength = occurencePointer - rxFrameBegining + 1;

			//check that rxbuffer contain all data which belong to message
			if((ESP_DeviceStatus.rxSize - ((occurencePointer + 1) - ESP_DeviceStatus.rxBuffer)) >= numberOfReceivedBytes)
			{
				//copy data to internal buffer
				ESP_CopyToInternalBuffer(numberOfSocket, (occurencePointer + 1), numberOfReceivedBytes);

				//shift all data to be sure that this memory area will not interpret as new frame
				ESP_DeleteDataFromRxBuffer(rxFrameBegining, headerLength + numberOfReceivedBytes);

				if(ESP_DeviceStatus.rxDeviceLock == true)
				{
					ESP_DeviceStatus.deviceStatus = READY;
					ESP_DeviceStatus.rxDeviceLock = false;
				}
			}
			else if(ESP_DeviceStatus.deviceStatus == READY)
			{
				ESP_DeviceStatus.deviceStatus = BUSY;
				ESP_DeviceStatus.rxDeviceLock = true;
				ESP_DeviceStatus.rxLockCounter = 0;
			}
			else if((ESP_DeviceStatus.rxDeviceLock == true) && (ESP_DeviceStatus.rxLockCounter >= ESP_DeviceStatus.rxLockTreshold))
			{
				ESP_DeviceStatus.deviceStatus = READY;
				ESP_DeviceStatus.rxDeviceLock = false;
				ESP_DeleteDataFromRxBuffer(rxFrameBegining, (ESP_DeviceStatus.rxSize - (rxFrameBegining - ESP_DeviceStatus.rxBuffer)) );
			}
		}/* if((occurencePointer != NULL) && (strstr(rxFrameBegining, ":") != NULL)) */
	}/* if (ESP_DeviceStatus.rxSize > MINIMAL_CHECKED_RX_SIZE) */

	//RX timeout (in READY state if no data will incomming in set duration time then clear RX buffer)
	if((RX_TIMEOUT_DISABLE != ESP_DeviceStatus.rxClearThreshold)
		&& (ESP_DeviceStatus.rxSize > 0))
	{
		if(ESP_DeviceStatus.rxClearCounter >= ESP_DeviceStatus.rxClearThreshold)
		{
			ESP_DeviceStatus.rxSize = 0;
			ESP_DeviceStatus.rxBuffer[0] = '\0';
			ESP_DeviceStatus.rxClearCounter = 0;
		}
		else if((ESP_DeviceStatus.deviceStatus == READY)
			&& (ESP_DeviceStatus.rxSize != ESP_DeviceStatus.lastRxSizeStatus))
		{
			ESP_DeviceStatus.rxClearCounter = 0;
		}
		else
		{
			ESP_DeviceStatus.rxClearCounter++;
		}

		ESP_DeviceStatus.lastRxSizeStatus = ESP_DeviceStatus.rxSize;
	}/* if(RX_TIMEOUT_DISABLE != ESP_DeviceStatus.rxClearThreshold) */

	//AT request timeout detection
	if((AT_REQ_TIMEOUT_DISABLE != ESP_DeviceStatus.timeoutRequestTreshold)
		&& (ESP_DeviceStatus.deviceStatus == BUSY)
		&& (ESP_DeviceStatus.timeoutRequestCounter >= ESP_DeviceStatus.timeoutRequestTreshold))
	{
		ESP_DeviceStatus.errorCode.AT_TIMEOUT = 1;
		ESP_DeviceStatus.deviceStatus = RESPONSE_RECEIVED;
		ESP_DeviceStatus.timeoutRequestCounter = 0;
	}

	//increment timeout counter when request is processed
	if(ESP_DeviceStatus.deviceStatus == BUSY)
		ESP_DeviceStatus.timeoutRequestCounter++;

}

/*****************************************************************************************
* ESP_GetRequestState() - return DEVICE_STATUS enum holding by ESP_DeviceStatus structure.
* DEVICE_STATUS enum contain information about current processed request. According to
* this information can be checked when request was finished.
*
* Return: DEVICE_STATUS enum holding by ESP_DeviceStatus structure with information
* about request state.
*****************************************************************************************/
DEVICE_STATUS ESP_GetRequestState(void)
{
	return ESP_DeviceStatus.deviceStatus;
}

/*****************************************************************************************
* ESP_SendDetectDeviceRequest() - copy simple AT command request to ESP layer TX buffer.
* This AT command don't change any option inside ESP8266 module but can be used to detect
* module which is used in this function. AT response for this command is constant.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
* 	AT\r\n
* Response:
*	AT\r\r\n\r\nOK\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendDetectDeviceRequest(void)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(4U);

		memcpy(&ESP_DeviceStatus.txBuffer[2], "\r\n", 2);

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_SendResetRequest() - copy reset AT command request to ESP layer TX buffer.
* Command will cause restart ESP8266 module and module with startup with default settings
* also settings set by user which is remembered also after reset cause by power and
* stored in flash memory.
* After response like below module send two parts of data first with baudrate the same like
* AT command was send. Second part was send with baudrate like on startup. Observation was
* performed on module on which baudrate wasn't changed. First part of data was added below:
*	\r\n ets Jan 8 2013,rst cause:2,boot mode:(3,7)\r\n\r\nload 0x40100000, len 1856, room 16 \r\ntail 0\r\nchksum 0x63\r\nload 0x3ffe8000, len 776, room 8 \r\ntail 0\r\nchksum 0x02\r\nload 0x3ffe8310, len 552, room 8
*	\r\ntail 0\r\nchksum 0x79\r\ncsum 0x79\r\n\r\n2nd boot version : 1.5\r\n  SPI Speed      : 40MHz\r\n  SPI Mode       : DIO\r\n  SPI Flash Size & Map: 8Mbit(512KB+512KB)\r\njump to run user1 @ 1000\r\n\r\n
* Time between begining of AT request and last data received from ESP module was measure
* and is equal 0.262s so wait procederu should be added.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
* 	AT+RST\r\n
* Response:
*	AT+RST\r\r\n\r\nOK\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendResetRequest(void)
{
	if (ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(8U);

		strcat(ESP_DeviceStatus.txBuffer, "RST\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_ProcessGaneralFormatResponse() - function used to process AT command response if it
* was received or if AT timeout was set. This type of analyze response can be used to many
* request which return simple answear.
*
* Return: true if device correctly response without any errors.
*****************************************************************************************/
bool ESP_ProcessGaneralFormatResponse(void)
{
	if(ESP_DeviceStatus.deviceStatus == RESPONSE_RECEIVED)
		ESP_DeviceStatus.deviceStatus = READY;

	if(*((uint32_t *)&ESP_DeviceStatus.errorCode) > 0)
		return false;
	else
		return true;
}

/*****************************************************************************************
* ESP_SendConnectToApnRequest() - initialize ESP layer to send AT command request with
* parameters necessary to connect to APN. Those data are SSID and password. Before using
* request appropriate mode should be set via function ESP_SendWifiModeRequest. If mode
* will be AT_SOFT_AP_MODE then connection failure.
* This request can be processed very long. During one test time between last bit of AT
* request and last bit of AT response is equal 7.000s in one test measure.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
*	AT+CWJAP_CUR="TestTest","289384759667"\r\n
* Response:
*	AT+CWJAP_CUR="TestTest","289384759667"\r\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n
*
* Parameters:
* @ssidName: SSID of chosen APN
* @password: password to chosen APN
*
* Return: true if during AT comunication any errors don't occur like UART errors or AT
* errors. Errors is hold in ESP_DeviceStatus.errorCode structure.
*****************************************************************************************/
bool ESP_SendConnectToApnRequest(const char* ssidName, const char* password)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(4U);

		strcat(ESP_DeviceStatus.txBuffer, "CWJAP_CUR=\"");
		strCatWithSpecialCharacters(ESP_DeviceStatus.txBuffer, ssidName);
		strcat(ESP_DeviceStatus.txBuffer, "\",\"");
		strCatWithSpecialCharacters(ESP_DeviceStatus.txBuffer, password);
		strcat(ESP_DeviceStatus.txBuffer, "\"\r\n");
		ESP_DeviceStatus.txSize = strlen(ESP_DeviceStatus.txBuffer);

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_SendWifiModeRequest() - store in ESP layer TX buffer AT request which will switch
* mode of ESP8266 module. Supported modes are: AT_STATION_MODE, AT_SOFT_AP_MODE and
* AT_SOFT_AP_STATION_MODE. Tested ESP-07 module default work in AT_SOFT_AP_MODE mode.
* Execute some operation in invalide mode can cause error during perform AT command request.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
* 	AT+CWMODE_CUR=1\r\n
* Response:
*	AT+CWMODE_CUR=1\r\r\n\r\nOK\r\n
*
* Parameters:
* @wifiMode: this parameter contain selected mode. Possible values and meaning of it is
*  stored inside ESP_DEVICE_MODE enum.
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendWifiModeRequest(uint8_t wifiMode)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(17U);

		strcat(ESP_DeviceStatus.txBuffer, "CWMODE_CUR=\0");

		if((wifiMode >= AT_STATION_MODE) && (wifiMode <= AT_SOFT_AP_STATION_MODE))
		{
			uint8_t modeChar = (uint8_t)('0') + wifiMode;
			strncat(ESP_DeviceStatus.txBuffer, &modeChar, 1);
		}

		strcat(ESP_DeviceStatus.txBuffer, "\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_SendDisconnectRequest() - if ESPESP8266 module is connected to APN this function
* can be used to copy AT command request ESP layer TX buffer.
* Process function:
*	ESP_ProcessApnListResponse()
* Request:
* 	AT+CWQAP\r\n
* Response:
* 	AT+CWQAP\r\r\n\r\nOK\r\nWIFI DISCONNECT\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendDisconnectRequest(void)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(10U);

		strcat(ESP_DeviceStatus.txBuffer, "CWQAP\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_SendApnListRequest() - store in ESP layer TX buffer AT request to get access points
* list. Performing AT command request with receive response can consume 2.14s(value was
* measure during one test case).
* Process function:
*	ESP_ProcessApnListResponse()
* Request:
*	AT+CWLAP\r\n
* Response:
*	AT+CWLAP\r\r\n+CWLAP:(4,"TestTest",-44,"94:f2:56:cb:80:da",10,-2,0)\r\n+CWLAP:(4,"TestTest2",-90,"da:84:69:d2:42:40",3,-4,0)\r\n\r\nOK\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendApnListRequest(void)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(10U);

		strcat(ESP_DeviceStatus.txBuffer, "CWLAP\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_ProcessApnListResponse() - function used to process AT command response with
* information about available access points. Obtained information via function is stored
* inside ApnStructure which is available as global structure.
*
* Return: true if during AT comunication any errors don't occur like UART errors or AT
* errors. Errors is hold in ESP_DeviceStatus.errorCode structure.
*****************************************************************************************/
bool ESP_ProcessApnListResponse(void)
{
	uint8_t *rxCwlapBufferPointer = ESP_DeviceStatus.rxBuffer;
	ApnStructure.NumberOfApn = 0;

	//fill structures in AppInformationTable
	for(uint16_t i = 0; i < MAX_NUMBER_OF_APN; i++)
	{
		rxCwlapBufferPointer = strstr(rxCwlapBufferPointer, "+CWLAP:");
		if(rxCwlapBufferPointer != NULL)
		{
			uint8_t* endingCharLocation = 0;

			//fill securityLevel
			ApnStructure.AppInformationTable[i].securityLevel = rxCwlapBufferPointer[8] - '0';

			//fill ssid
			endingCharLocation = strstr(&rxCwlapBufferPointer[11], "\",");

			if(endingCharLocation != NULL)
			{
				memcpy(ApnStructure.AppInformationTable[i].ssid, &rxCwlapBufferPointer[11],
						(int)(endingCharLocation - &rxCwlapBufferPointer[11]));
				//add NULL termination symbol
				ApnStructure.AppInformationTable[i].ssid[endingCharLocation - &rxCwlapBufferPointer[11]] = '\0';
			}
			else
			{
				break;
			}

			//fill signalPower
			endingCharLocation = strstr(&rxCwlapBufferPointer[11], "\",");

			if(endingCharLocation != NULL)
			{
				ApnStructure.AppInformationTable[i].signalPower = atoi(endingCharLocation + 2);
				rxCwlapBufferPointer = endingCharLocation + 2;
			}
			else
			{
				break;
			}

			//fill bssid
			endingCharLocation = strstr(rxCwlapBufferPointer, ",\"");

			if(endingCharLocation != NULL)
			{
				memcpy(ApnStructure.AppInformationTable[i].bssid, endingCharLocation + 2, 17);
				ApnStructure.AppInformationTable[i].bssid[17] = '\0';
			}
			else
			{
				break;
			}

			//increasy counter with APN number
			ApnStructure.NumberOfApn++;
		}
		else/* in this case it mean that no more access point wasn't detected */
		{
			break;
		}
	}

	if(ESP_DeviceStatus.deviceStatus == RESPONSE_RECEIVED)
		ESP_DeviceStatus.deviceStatus = READY;

	if(*((uint32_t *)&ESP_DeviceStatus.errorCode) > 0)
		return false;
	else
		return true;
}

/*****************************************************************************************
* ESP_GetAssignedIpAddress() - store in ESP layer TX buffer AT request to get
* IP address assigned to device.
* Process function:
*	ESP_ProcessGetAssignedIpResponse()
* Request:
*	AT+CIFSR\r\n
* Response:
*	AT+CIFSR\r\r\n+CIFSR:STAIP,"192.168.1.3"\r\n+CIFSR:STAMAC,"8C:FC:8F:66:30:6F"\r\n\r\nOK\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_GetAssignedIpAddress(void)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(10U);

		strcat(ESP_DeviceStatus.txBuffer, "CIFSR\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_ProcessGetAssignedIpResponse() - function used to process AT command response with
* information about IP address assigned to device. IP address is copied to memory pointed
* by function parameter(pointerToIpAdressTable).
*
* Parameters:
* @pointerToIpAdressTable: pointer to memory where decoded IP address will be copied.
*
* Return: true if during AT comunication any errors don't occur like UART errors or AT
* errors. Errors is hold in ESP_DeviceStatus.errorCode structure.
*****************************************************************************************/
bool ESP_ProcessGetAssignedIpResponse(uint8_t *pointerToIpAdressTable)
{
	uint8_t ipAddressTable[IP_ADDRESS_BYTE_LENGTH];
	uint8_t *rxIpBufferPointer = NULL;

	if(ESP_DeviceStatus.deviceStatus == RESPONSE_RECEIVED)
		ESP_DeviceStatus.deviceStatus = READY;

	//check errors to don't parse possible incorrect string
	if(*((uint32_t *)&ESP_DeviceStatus.errorCode) > 0)
	{
		return false;
	}

	rxIpBufferPointer = strstr(ESP_DeviceStatus.rxBuffer, "STAIP");

	if(rxIpBufferPointer == NULL)
	{
		rxIpBufferPointer = strstr(ESP_DeviceStatus.rxBuffer, "APIP");
	}

	if(rxIpBufferPointer != NULL)
	{
		//parse IP address
		rxIpBufferPointer = strstr(rxIpBufferPointer, "\"");

		if(!ESP_ParseIpAddress(rxIpBufferPointer, ipAddressTable))
		{
			return false;
		}
	}/* if(rxIpBufferPointer != NULL) */

	memcpy(pointerToIpAdressTable, ipAddressTable, 4);

	return true;
}

/*****************************************************************************************
* ESP_GetConnectionStatus() - store in ESP layer TX buffer AT request to get connection
* status. Connection status in this case mean that ESP module is connected via WiFi to
* other device or not. Execution of this AT command request also cause that in response
* can be added information about links(connections to sockets) if links will established.
* This mean that SocketStateTable will be updated.
* Process function:
*	ESP_ProcessConnectionStatus()
* Request:
*	AT+CIPSTATUS\r\n
* Response:
*	AT+CIPSTATUS\r\r\nSTATUS:2\r\n\r\nOK\r\n
*	AT+CIPSTATUS\r\r\nSTATUS:3\r\n+CIPSTATUS:0,"TCP","192.168.1.3",51171,3000,1\r\n\r\nOK\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_GetConnectionStatus(void)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(14U);

		strcat(ESP_DeviceStatus.txBuffer, "CIPSTATUS\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_ProcessConnectionStatus() - function used to process AT command response with
* information about connection status. If returned status as AT response will be equal
* 5(station not connected to access point) then to variable pointed by flagPointer(function
* input parameter) will be copied false, otherwise will be copied true. If returned status
* as AT response will be equal 3 (station created TCP or UPD trasmnission) then link
* information in SocketStateTable will be updated.
*
* Parameters:
* @flagPointer: pointer to flag which will be store connection status. True mean that
*  device is connected via WiFi to other device.
*
* Return: true if during AT comunication any errors don't occur like UART errors or AT
* errors. Errors is hold in ESP_DeviceStatus.errorCode structure.
*****************************************************************************************/
bool ESP_ProcessConnectionStatus(bool *flagPointer)
{
	uint8_t *rxStatusBufferPointer = NULL;
	volatile uint8_t linkStatus = 0;

	if(ESP_DeviceStatus.deviceStatus == RESPONSE_RECEIVED)
		ESP_DeviceStatus.deviceStatus = READY;

	//check errors to don't parse possible incorrect string
	if(*((uint32_t *)&ESP_DeviceStatus.errorCode) > 0)
	{
		return false;
	}

	rxStatusBufferPointer = strstr(ESP_DeviceStatus.rxBuffer, "STATUS:");

	if(rxStatusBufferPointer != NULL)
	{
		linkStatus = rxStatusBufferPointer[7] - (uint8_t)'0';

		if(linkStatus == STATUS_STATION_NOT_CONNECTED_TO_APN)
		{
			*flagPointer = false;
		}
		else
		{
			*flagPointer = true;
		}

		//clear
		for(uint8_t i = 0; i < MAX_NUMBER_OF_SOCKET; i++)
		{
			SocketStateTable[i].socketIsOpen = false;
			SocketStateTable[i].additionalSocketDataIsAvailable = false;
		}

		if(linkStatus == STATUS_STATION_CREATED_TCP_OR_UDP_TRANSMISSION)
		{
			for(uint8_t i = 0; i < MAX_NUMBER_OF_SOCKET; i++)
			{
				rxStatusBufferPointer = strstr(rxStatusBufferPointer, "CIPSTATUS:");

				//pass this condition mean that entries with information about socket exist
				if(rxStatusBufferPointer != NULL)
				{
					uint8_t socketId = rxStatusBufferPointer[10] - (uint8_t)'0';

					rxStatusBufferPointer = &rxStatusBufferPointer[13];

					if(socketId > (MAX_NUMBER_OF_SOCKET - 1))
						break;

					//parse protocol type
					if(rxStatusBufferPointer[0] == 'T')
					{
						SocketStateTable[socketId].connectionType = TCP_CONNECTION_TYPE;
					}
					else if(rxStatusBufferPointer[0] == 'U')
					{
						SocketStateTable[socketId].connectionType = UDP_CONNECTION_TYPE;
					}
					else
					{
						break;
					}

					//parse IP address
					rxStatusBufferPointer = strstr(rxStatusBufferPointer, ",\"");

					if(rxStatusBufferPointer != NULL)
					{
						rxStatusBufferPointer++;

						if(!ESP_ParseIpAddress(rxStatusBufferPointer, SocketStateTable[socketId].remoteIpAddress))
						{
							break;
						}

					}/* if(rxIpBufferPointer != NULL) */
					else
					{
						break;
					}

					//parse remote port
					rxStatusBufferPointer = strstr(rxStatusBufferPointer, "\",");

					if(rxStatusBufferPointer != NULL)
					{
						SocketStateTable[socketId].remotePort = atoi(rxStatusBufferPointer + 2);
						rxStatusBufferPointer += 2;
					}
					else
					{
						break;
					}

					//parse local port
					rxStatusBufferPointer = strstr(rxStatusBufferPointer, ",");

					if(rxStatusBufferPointer != NULL)
					{
						SocketStateTable[socketId].localPort = atoi(rxStatusBufferPointer + 1);
						rxStatusBufferPointer++;
					}
					else
					{
						break;
					}

					//parse tcp client or server
					rxStatusBufferPointer = strstr(rxStatusBufferPointer, ",");

					if(rxStatusBufferPointer != NULL)
					{
						uint8_t tcpConnectionTypeTmp = atoi(rxStatusBufferPointer + 1);
						if(tcpConnectionTypeTmp == 0)
							SocketStateTable[socketId].tcpConnectionRunAsServer = false;
						else if(tcpConnectionTypeTmp == 1)
							SocketStateTable[socketId].tcpConnectionRunAsServer = true;
						else
							break;

					}
					else
					{
						break;
					}

					//mark that all data are correct
					SocketStateTable[i].socketIsOpen = true;
					SocketStateTable[i].additionalSocketDataIsAvailable = true;
				}/* if(rxStatusBufferPointer != NULL) */
				else
				{
					break;
				}
			}/* for(uint8_t i = 0; i < MAX_NUMBER_OF_SOCKET; i++) */
		}/* if(linkStatus == STATUS_STATION_CREATED_TCP_OR_UDP_TRANSMISSION) */
	}/* if(rxStatusBufferPointer != NULL) */

	return true;
}

/*****************************************************************************************
* ESP_SendAcceptMultipleConnectionRequest() - store in ESP layer TX buffer AT request to
* allow multiply connection to TCP server. Call this command is require for other function
* because only multiply connection is supported.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
*	AT+CIPMUX=1\r\n
* Response:
*	AT+CIPMUX=1\r\r\n\r\nOK\r\n
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendAcceptMultipleConnectionRequest(void)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(13U);

		strcat(ESP_DeviceStatus.txBuffer, "CIPMUX=1\r\n");

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_SendServerCommandRequest() - store in ESP layer TX buffer AT request to create or
* close TCP server. Function contain two parameters one to decide about close or open,
* second parameter decide about port number of server.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
*	AT+CIPSERVER=1,3000\r\n
* Response:
*	AT+CIPSERVER=1,3000\r\r\n\r\nOK\r\n
*
* Parameters:
* @serverStatus: flag which decide about action regarding to server. True mean that server
*  will be created. False meam that server will be closed.
* @portNumber: port number of created/closed server.
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendServerCommandRequest(bool serverStatus, uint16_t portNumber)
{
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		char portNumberTmp[8];
		memset(portNumberTmp, 0, sizeof(portNumberTmp));

		ESP_DataSendInit(0U);

		strcat(ESP_DeviceStatus.txBuffer, "CIPSERVER=");
		
		if(serverStatus)
			strcat(ESP_DeviceStatus.txBuffer, "1,");
		else
			strcat(ESP_DeviceStatus.txBuffer, "0,");
			
		strcat(ESP_DeviceStatus.txBuffer, itoa(portNumber, portNumberTmp, 10U));

		strcat(ESP_DeviceStatus.txBuffer, "\r\n");

		ESP_DeviceStatus.txSize = strlen(ESP_DeviceStatus.txBuffer);

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_SendWriteDataRequest() - store in ESP layer TX buffer AT request to send number of
* bytes to selected link ID(socketNumber). This function is one of two part send data
* process. To send data via socket is necessary use this function and in next if AT response
* will be OK then use ESP_Write function.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
*	AT+CIPSEND=0,18\r\n
* Response:
* 	"AT+CIPSEND=0,18\r\r\n\r\nOK\r\n> "
*
* Parameters:
* @socketNumber: value of link ID to which will be send data.
* @writeBufferSizeOf: number of bytes send to selected socketNumber(link ID).
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_SendWriteDataRequest(uint8_t socketNumber, uint16_t writeBufferSizeOf)
{
	//execute when device is ready and socket is open
	if(ESP_DeviceStatus.deviceStatus == READY && SocketStateTable[socketNumber].socketIsOpen == true)
	{
		char stringTmp[8];
		memset(stringTmp, 0, sizeof(stringTmp));

		ESP_DataSendInit(0U);

		strcat(ESP_DeviceStatus.txBuffer, "CIPSEND=");
		
		strcat(ESP_DeviceStatus.txBuffer, itoa(socketNumber, stringTmp, 10U));

		strcat(ESP_DeviceStatus.txBuffer, ",");

		strcat(ESP_DeviceStatus.txBuffer, itoa(writeBufferSizeOf, stringTmp, 10U));
		
		strcat(ESP_DeviceStatus.txBuffer, "\r\n");

		ESP_DeviceStatus.txSize = strlen(ESP_DeviceStatus.txBuffer);

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy or socket is closed so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_Write() - store in ESP layer TX buffer raw data which will be send to previously
* selected link ID. This function is one of two part send data process. First part is
* ESP_SendWriteDataRequest. This function is second part of send process.
* Process function:
*	ESP_ProcessGaneralFormatResponse()
* Request:
*	Example raw data: '0','1','0','0','0','0','0',\n,'0','0','0','0','1','1','128','0',\,'142'
* Response:
*	\r\nRecv 18 bytes\r\n\r\nSEND OK\r\n
*
* Parameters:
* @bufferPointer: pointer to buffer which hold data for transmission.
* @bufferSizeOf: number of data which must be copied from selected memory.
*
* Return: true if ESP layer is ready for new AT command request in another case return
* false.
*****************************************************************************************/
bool ESP_Write(uint8_t* bufferPointer, uint16_t bufferSizeOf)
{
	//execute when device is ready and socket is open
	if(ESP_DeviceStatus.deviceStatus == READY)
	{
		ESP_DataSendInit(bufferSizeOf);
		memcpy(ESP_DeviceStatus.txBuffer, bufferPointer, bufferSizeOf);

		//command will be executed when function ESP_Process() will call
		return true;
	}
	else
	{
		//device is busy or socket is closed so AT command cannot be executed
		return false;
	}
}

/*****************************************************************************************
* ESP_ClearRxBuffer() - clear RX data and status register inside physical UART port used
* by ESP layer.
*
*****************************************************************************************/
void ESP_ClearRxBuffer(void)
{
	UART_Status status = UART_ReturnStatusRegister(ESP_DeviceStatus.uartPortNumber);

	//clear UART RX buffer
	for(uint8_t rxDataTmp = 0; status.RDR == 1;)
	{
		rxDataTmp = UART_ReadByteFromTrasmitter(ESP_DeviceStatus.uartPortNumber);
		status = UART_ReturnStatusRegister(ESP_DeviceStatus.uartPortNumber);
	}
}

/*****************************************************************************************
* ESP_GetUartPortNumber() - function return UART port number used by ESP layer. The same port
* number was set during call initialization function(ESP_Init()).
*
* Return: UART port number used by ESP layer.
*****************************************************************************************/
uint8_t ESP_GetUartPortNumber(void)
{
	return ESP_DeviceStatus.uartPortNumber;
}

/*****************************************************************************************
* USART0_IRQHandler() - function call if interruption on UART port occur. If code inside
* this file will be move to other microntroller this function must be changed or removed.
*
*****************************************************************************************/
void USART0_IRQHandler(void)
{
	ESP_Process();
}
