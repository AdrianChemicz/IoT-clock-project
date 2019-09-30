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

#include "WIFI_InteractionLayer.h"
#include "Thread.h"
#include "UART_Driver.h"
#include <string.h>

static uint16_t MaxMethodIdInServiceDayMeasurement = 0;

/*****************************************************************************************
* WIFI_Init() - function initialize WiFi module by check availability, reset and set
* appropriate mode. Inside this function WifiStateType structure isn't used but more
* important result of WiFi initialization is stored in ClockState.
*****************************************************************************************/
void WIFI_Init(void)
{
	static bool sendRequestFlag = false;
	static uint8_t sequenceCommandState = WIFI_STARTUP_WAIT;

	//initialize variable
	if(MaxMethodIdInServiceDayMeasurement == 0)
	{
		MaxMethodIdInServiceDayMeasurement = sizeof(TemperatureSingleDayRecordType) / SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE;

		if(sizeof(TemperatureSingleDayRecordType)%SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE != 0)
		{
			MaxMethodIdInServiceDayMeasurement++;
		}
	}

	if(sendRequestFlag == false)
	{
		switch(sequenceCommandState)
		{
		case WIFI_STARTUP_WAIT://wait two second for clear RX buffer
		case WIFI_RESET_WAIT://wait two second for clear RX buffer
		case WIFI_DEVICE_DONT_DETECTED:

			break;

		case WIFI_SEND_FIRST_REQUEST:
			ESP_SendDetectDeviceRequest();
			break;

		case WIFI_SEND_RESET:
			ESP_SendResetRequest();
			break;

		case WIFI_SEND_ESP_MODE:
			ESP_SendWifiModeRequest(AT_STATION_MODE);
			break;

		case WIFI_SEND_DETECT_REQUEST:
			ESP_SendDetectDeviceRequest();
			break;

		default:

			break;

		}

		sendRequestFlag = true;
	}/* if(sendRequestFlag == false) */

	if((sequenceCommandState == WIFI_STARTUP_WAIT)
		|| (sequenceCommandState == WIFI_RESET_WAIT))
	{
		static uint16_t wifiDelayCounter = 0;

		wifiDelayCounter++;

		if(wifiDelayCounter > 2*ONE_SECONDS)
		{
			ESP_ClearRxBuffer();

			sequenceCommandState++;
			wifiDelayCounter = 0;
			sendRequestFlag = false;

			return;
		}
	}
	else
	{
		ESP_Process();

		if(ESP_GetRequestState() == RESPONSE_RECEIVED)
		{
			bool returnRequestState = ESP_ProcessGaneralFormatResponse();

			if(sequenceCommandState == WIFI_SEND_DETECT_REQUEST)
			{
				ClockState.wifiReady = returnRequestState;
			}

			sequenceCommandState++;
			sendRequestFlag = false;
		}
	}
}

/*****************************************************************************************
* WIFI_ProcessRequest() - function is only call from WIFI_Process and is only responsible for
*	process SOME/IP request. If SOME/IP message is correct(correct SOME/IP message header
*	and CRC if this extension was set) then payload is processed. Inside this function is
*	performed request validation like chech that service and method is supported and data
*	for dedicated request is correct.
*
* Parameters:
* @wifiStateStructure: pointer to WifiStateType structure which contain data like txBuffers,
*  request data and buffer used for FRAM requests proces.
*
*****************************************************************************************/
static void WIFI_ProcessRequest(WifiStateType* wifiStateStructure)
{
	if(ClockState.wifiConnected && wifiStateStructure->initSocketFlag
		&& wifiStateStructure->messageIsReceived)
	{
		//check that received message contain SOME_IP_REQUEST_CODE
		if(wifiStateStructure->someIpReceivedMessage.messageType == SOME_IP_REQUEST_CODE)
		{
			switch(wifiStateStructure->someIpReceivedMessage.serviceId)
			{
			case SOME_IP_SERVICE_CLOCK_STATUS:
				switch(wifiStateStructure->someIpReceivedMessage.methodId)
				{
				case SOME_IP_SERVICE_CLOCK_STATUS_METHOD_SET:
					//validate set request
					{
						bool someIpPayloadErrorOccur = false;
						SomeIpClockStatusSetRequestMethodPayload *requestPayloadStructure = (SomeIpClockStatusSetRequestMethodPayload*)wifiStateStructure->someIpReceivedMessage.payload;

						if(wifiStateStructure->someIpReceivedMessage.payloadSize != SOME_IP_SERVICE_CLOCK_STATUS_METHOD_SET_PAYLOAD_SIZE)
						{
							someIpPayloadErrorOccur = true;
						}
						else
						{
							if(requestPayloadStructure->minute > 60 || requestPayloadStructure->hour > 23
									|| requestPayloadStructure->month > 12 || requestPayloadStructure->year > 254)
							{
								someIpPayloadErrorOccur = true;
							}
							else
							{
								uint8_t dayTmp = GUI_ReturnMaxDayInMonth(requestPayloadStructure->month, requestPayloadStructure->year);

								if(dayTmp < requestPayloadStructure->day)
								{
									someIpPayloadErrorOccur = true;
								}
							}
						}

						if(someIpPayloadErrorOccur)
						{
							wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
									wifiStateStructure->someIpReceivedMessage.serviceId,
									wifiStateStructure->someIpReceivedMessage.methodId,
									SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_MALFORMED_MESSAGE,
									wifiStateStructure->TxMessageTable[0].payload , NULL, 0);

							wifiStateStructure->TxMessageTable[0].lockFlag = true;
						}
						else
						{
							ClockState.currentTimeSecond = 0;
							ClockState.currentTimeMinute = requestPayloadStructure->minute;
							ClockState.currentTimeHour = requestPayloadStructure->hour;
							ClockState.day = requestPayloadStructure->day;
							ClockState.month = requestPayloadStructure->month;
							ClockState.year = requestPayloadStructure->year;

							wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
									wifiStateStructure->someIpReceivedMessage.serviceId,
									wifiStateStructure->someIpReceivedMessage.methodId,
									SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_OK_VALUE,
									wifiStateStructure->TxMessageTable[0].payload , NULL, 0);

							wifiStateStructure->TxMessageTable[0].lockFlag = true;
						}
					}
					break;

				case SOME_IP_SERVICE_CLOCK_STATUS_METHOD_GET:
					{
						SomeIpClockStatusGetResponseMethodPayload getResponseMethodPayload;

						getResponseMethodPayload.minute = ClockState.currentTimeMinute;
						getResponseMethodPayload.hour = ClockState.currentTimeHour;
						getResponseMethodPayload.day = ClockState.day;
						getResponseMethodPayload.month = ClockState.month;
						getResponseMethodPayload.year = ClockState.year;
						getResponseMethodPayload.reserved = 0;
						getResponseMethodPayload.temperatureOutside = ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValue;
						getResponseMethodPayload.temperatureInside = ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValue;
						getResponseMethodPayload.temperatureFurnace = ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue;

						wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
								wifiStateStructure->someIpReceivedMessage.serviceId,
								wifiStateStructure->someIpReceivedMessage.methodId,
								SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_OK_VALUE,
								wifiStateStructure->TxMessageTable[0].payload , (uint8_t *)&getResponseMethodPayload,
								SOME_IP_SERVICE_CLOCK_STATUS_METHOD_GET_PAYLOAD_SIZE);

						wifiStateStructure->TxMessageTable[0].lockFlag = true;
					}
					break;
					//not supported method ID
				default:
					wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
							wifiStateStructure->someIpReceivedMessage.serviceId,
							wifiStateStructure->someIpReceivedMessage.methodId,
							SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_MALFORMED_MESSAGE,
							wifiStateStructure->TxMessageTable[0].payload , NULL, 0);

					wifiStateStructure->TxMessageTable[0].lockFlag = true;

					break;
				}
				break;

			case SOME_IP_SERVICE_DAY_MEASUREMENT:
				//validate get temperature request
				{
					bool someIpPayloadErrorOccur = false;
					SomeIpDayMeasurmentRequestPayload *requestPayloadStructure = (SomeIpDayMeasurmentRequestPayload*)wifiStateStructure->someIpReceivedMessage.payload;

					if(wifiStateStructure->someIpReceivedMessage.payloadSize != SOME_IP_SERVICE_DAY_MEASUREMENT_REQ_PAYLOAD_SIZE)
					{
						someIpPayloadErrorOccur = true;
					}
					else
					{
						if(requestPayloadStructure->month > 12 || requestPayloadStructure->year > 254)
						{
							someIpPayloadErrorOccur = true;
						}
						else
						{
							uint8_t dayTmp = GUI_ReturnMaxDayInMonth(requestPayloadStructure->month, requestPayloadStructure->year);

							if(dayTmp < requestPayloadStructure->day)
							{
								someIpPayloadErrorOccur = true;
							}
						}
					}

					if(someIpPayloadErrorOccur)
					{
						wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
								wifiStateStructure->someIpReceivedMessage.serviceId,
								wifiStateStructure->someIpReceivedMessage.methodId,
								SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_MALFORMED_MESSAGE,
								wifiStateStructure->TxMessageTable[0].payload , NULL, 0);

						wifiStateStructure->TxMessageTable[0].lockFlag = true;
					}
					else
					{
						/*
						 * Method ID is used as index of structure. Structure is too big to send
						 * in one message so is send as parts. Size of message is defined by define
						 * SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE. If method ID
						 * multiply with above define exceed structure size then it must be raise
						 * as unknown method.
						 */
						if(wifiStateStructure->someIpReceivedMessage.methodId > MaxMethodIdInServiceDayMeasurement)
						{
							wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
									wifiStateStructure->someIpReceivedMessage.serviceId,
									wifiStateStructure->someIpReceivedMessage.methodId,
									SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_UNKNOWN_METHOD,
									wifiStateStructure->TxMessageTable[0].payload , NULL, 0);

							wifiStateStructure->TxMessageTable[0].lockFlag = true;

							break;
						}

						//copy searched header from request
						memcpy(&wifiStateStructure->SearchedDayMeasurementHeader, requestPayloadStructure, DAY_MEASUREMENT_HEADER_SIZE);

						//check that searched structure don't belong to current day and record this type of structure is enable
						if((ClockState.day == wifiStateStructure->SearchedDayMeasurementHeader.day)
							&& (ClockState.month == wifiStateStructure->SearchedDayMeasurementHeader.month)
							&& (ClockState.year == wifiStateStructure->SearchedDayMeasurementHeader.year)
							&& ClockState.TemperatureSensorTable[wifiStateStructure->SearchedDayMeasurementHeader.source].recordTemperature)
						{
							wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer
								= TemperatureSingleDay[wifiStateStructure->SearchedDayMeasurementHeader.source];
							wifiStateStructure->temperatureStructureIsValid = true;

							wifiStateStructure->searchState = SEARCH_FINISHED;
						}
						//check that searched structure isn't stored in buffer
						else if(wifiStateStructure->temperatureStructureIsValid
							&& (wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer.day
								== wifiStateStructure->SearchedDayMeasurementHeader.day)
							&& (wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer.month
								== wifiStateStructure->SearchedDayMeasurementHeader.month)
							&& (wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer.year
								== wifiStateStructure->SearchedDayMeasurementHeader.year)
							&& (wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer.source
								== wifiStateStructure->SearchedDayMeasurementHeader.source))
						{
							wifiStateStructure->searchState = SEARCH_FINISHED;
						}
						else
						{
							wifiStateStructure->searchState = SEARCH_REQUESTED;
						}
					}
				}
				break;

				//service isn't supported
			default:

				wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
						wifiStateStructure->someIpReceivedMessage.serviceId,
						wifiStateStructure->someIpReceivedMessage.methodId,
						SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_UNKNOWN_SERVICE,
						wifiStateStructure->TxMessageTable[0].payload , NULL, 0);

				wifiStateStructure->TxMessageTable[0].lockFlag = true;

				break;
			}

			wifiStateStructure->messageIsReceived = false;
		}/* if(wifiStateStructure->someIpReceivedMessage.messageType == SOME_IP_REQUEST_CODE) */
	}/* if(ClockState.wifiConnected && wifiStateStructure->initSocketFlag
		&& wifiStateStructure->messageIsReceived) */
}

/*****************************************************************************************
* WIFI_Process() - function must be call cyclically after initialization process with correct
*	result. Information about get connection status, assigned IP address and available APN
*	is performed inside this function. Inside this part is also performed log to APN if
*	password was set.
*
* Parameters:
* @wifiStateStructure: pointer to.WifiStateType structure with data like counters used to
*  decide when send get access points or check connection status.
*
*****************************************************************************************/
void WIFI_Process(WifiStateType* wifiStateStructure)
{
	//manage connection state
	if(wifiStateStructure->processedRequest == (uint8_t)WIFI_REQUEST_NONE)
	{
		//Connect to APN
		if((ClockState.wifiConnected == false)
			&& (strlen(ClockState.ssidOfAssignedApn) != 0)
			&& (wifiStateStructure->connectToApnCounter > ONE_SECONDS*30)
			&& ESP_SendConnectToApnRequest(ClockState.ssidOfAssignedApn, ClockState.passwordToAssignedApn))
		{
			wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_CONNECT_TO_APN;
			wifiStateStructure->connectToApnCounter = 0;
		}
		//send get APN list request
		else if((gui.active_window == &wifiSettingsWindow)
				&& (wifiStateStructure->getApnCounter > ONE_SECONDS*15)
				&& ESP_SendApnListRequest())
		{
			wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_GET_APN;
			wifiStateStructure->getApnCounter = 0;
		}
		//check that disconnect request isn't active
		else if(ClockState.wifiStartDisconnect
				&& ESP_SendDisconnectRequest())
		{
			wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_DISCONNECT;
		}
		//get IP address
		else if(ClockState.wifiConnected && ClockState.ipAddressAssignedToDevice[0] == 0
				&& ESP_GetAssignedIpAddress())
		{
			wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_GET_IP_ADDRESS;
		}
		//check connection status
		else if(ClockState.wifiConnected && (wifiStateStructure->checkConnectionCounter > ONE_SECONDS*10)
				&& ESP_GetConnectionStatus())
		{
			wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_CONNECTION_STATUS;
			wifiStateStructure->checkConnectionCounter = 0;
		}
		//after succesfut connection initialize sockets
		else if(ClockState.wifiConnected && (wifiStateStructure->initSocketFlag == false))
		{
			switch(wifiStateStructure->initSocketPhase)
			{
			case WIFI_INIT_MULTIPLY_CONNECTION:
				if(ESP_SendAcceptMultipleConnectionRequest())
				{
					wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_ACCEPT_MULLTIPLY_CONNECTION;
				}
				break;

			case WIFI_CREATE_TCP_SERVER:
				if(ESP_SendServerCommandRequest(true, TCP_SERVER_PORT_NUMBER))
				{
					wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_CREATE_TCP_SERVER;
				}

				break;

			default:
				break;
			}
		}
	}/* if(wifiStateStructure->processedRequest == (uint8_t)WIFI_REQUEST_NONE) */

	//process data received and send by device
	if(ClockState.wifiConnected && wifiStateStructure->initSocketFlag
		&& (wifiStateStructure->processedRequest == (uint8_t)WIFI_REQUEST_NONE))
	{
		//search new request
		for(uint16_t i = 0; i < MAX_NUMBER_OF_RX_BUFFER; i++)
		{
			if(RxMessageTable[i].lockFlag == true)
			{
				SomeIpMessage someIpMessageTmp;

				if(SOMEIP_DecodeRxMessage(&someIpMessageTmp, NULL, RxMessageTable[i].payload, RxMessageTable[i].payloadSize))
				{
					if(wifiStateStructure->messageIsReceived == false)
					{
						wifiStateStructure->someIpReceivedMessage = someIpMessageTmp;
						wifiStateStructure->someIpReceivedMessage.socketId = RxMessageTable[i].socketId;
						wifiStateStructure->messageIsReceived = true;
					}
					else
					{

					}
				}

				RxMessageTable[i].lockFlag = false;
			}/* if(RxMessageTable[i].lockFlag == true) */
		}/* for(int i = 0; i < MAX_NUMBER_OF_RX_BUFFER; i++) */
	}/* if(ClockState.wifiConnected && wifiStateStructure->initSocketFlag) */

	WIFI_ProcessRequest(wifiStateStructure);

	if((wifiStateStructure->processedRequest == (uint8_t)WIFI_REQUEST_NONE)
		&& ClockState.wifiConnected && wifiStateStructure->initSocketFlag)
	{
		for(uint16_t i = 0; i < MAX_NUMBER_OF_TX_BUFFER; i++)
		{
			if(wifiStateStructure->TxMessageTable[i].lockFlag == true)
			{
				if(ESP_SendWriteDataRequest(wifiStateStructure->TxMessageTable[i].socketId,
					wifiStateStructure->TxMessageTable[i].payloadSize))
				{
					wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_SEND_RESPONSE_MESSAGE;
					wifiStateStructure->sendTxMessageNumber = i;
				}

				break;
			}
		}
	}/* if((wifiStateStructure->processedRequest == (uint8_t)WIFI_REQUEST_NONE)
		&& ClockState.wifiConnected && wifiStateStructure->initSocketFlag) */

	wifiStateStructure->connectToApnCounter++;
	wifiStateStructure->getApnCounter++;

	if(ClockState.wifiConnected)
	{
		wifiStateStructure->checkConnectionCounter++;
	}

	UART_DisableInterrupts(ESP_GetUartPortNumber());
	ESP_Process();
	UART_EnableInterrupts(ESP_GetUartPortNumber());

	if(ESP_GetRequestState() == (uint8_t)RESPONSE_RECEIVED)
	{
		switch(wifiStateStructure->processedRequest)
		{
		case WIFI_REQUEST_CONNECT_TO_APN:
			ClockState.wifiConnected = ESP_ProcessGaneralFormatResponse();

			if(ClockState.wifiConnected)
			{
				wifiStateStructure->checkConnectionCounter = 0;
			}
			break;

		case WIFI_REQUEST_GET_APN:
			ClockState.wifiApnReceived = ESP_ProcessApnListResponse();
			break;

		case WIFI_REQUEST_DISCONNECT:
			if(ESP_ProcessApnListResponse())
			{
				ClockState.wifiStartDisconnect = false;
			}
			else
			{
				wifiStateStructure->diconnectRepetitionCounter++;

				//case when call disconnect isn't possible few time
				if(wifiStateStructure->diconnectRepetitionCounter > 3)
				{
					ClockState.wifiStartDisconnect = false;
					wifiStateStructure->diconnectRepetitionCounter = 0;
				}
			}
			break;

		case WIFI_REQUEST_GET_IP_ADDRESS:
			ESP_ProcessGetAssignedIpResponse(ClockState.ipAddressAssignedToDevice);
			break;

		case WIFI_REQUEST_CONNECTION_STATUS:
			ESP_ProcessConnectionStatus(&ClockState.wifiConnected);
			wifiStateStructure->checkConnectionCounter = 0;

			//connection was lost
			if(ClockState.wifiConnected == false)
			{
				memset(ClockState.ipAddressAssignedToDevice, 0, 4);
				ClockState.wifiStartDisconnect = false;
			}
			break;

		case WIFI_REQUEST_ACCEPT_MULLTIPLY_CONNECTION:
			if(ESP_ProcessGaneralFormatResponse())
			{
				wifiStateStructure->initSocketPhase = WIFI_CREATE_TCP_SERVER;
			}
			break;

		case WIFI_REQUEST_CREATE_TCP_SERVER:
			if(ESP_ProcessGaneralFormatResponse())
			{
				wifiStateStructure->initSocketFlag = true;
			}
			break;

		case WIFI_REQUEST_SEND_RESPONSE_MESSAGE:
			if(ESP_ProcessGaneralFormatResponse())
			{
				if(wifiStateStructure->sendTxFirstStepWasPerformed == false)
				{
					ESP_Write(wifiStateStructure->TxMessageTable[wifiStateStructure->sendTxMessageNumber].payload,
						wifiStateStructure->TxMessageTable[wifiStateStructure->sendTxMessageNumber].payloadSize);

					wifiStateStructure->sendTxFirstStepWasPerformed = true;

					return;
				}
				else//transmission was performed with success
				{
					wifiStateStructure->TxMessageTable[wifiStateStructure->sendTxMessageNumber].lockFlag = false;

					//clear thing for send state machine
					wifiStateStructure->sendTxMessageNumber = 0;
					wifiStateStructure->sendTxFirstStepWasPerformed = false;
					wifiStateStructure->sendTxErrorCounter = 0;
				}
			}
			else
			{
				wifiStateStructure->sendTxErrorCounter++;

				//transmition was repeated few times but without success
				if(wifiStateStructure->sendTxErrorCounter > WIFI_SEND_DATA_REPETITION)
				{

					wifiStateStructure->TxMessageTable[wifiStateStructure->sendTxMessageNumber].lockFlag = false;

					//clear thing for send state machine
					wifiStateStructure->sendTxMessageNumber = 0;
					wifiStateStructure->sendTxFirstStepWasPerformed = false;
					wifiStateStructure->sendTxErrorCounter = 0;
				}
				else
				{
					ESP_SendWriteDataRequest(wifiStateStructure->TxMessageTable[wifiStateStructure->sendTxMessageNumber].socketId,
						wifiStateStructure->TxMessageTable[wifiStateStructure->sendTxMessageNumber].payloadSize);

					return;
				}
			}
			break;

		default:

			break;
		}

		wifiStateStructure->processedRequest = (uint8_t)WIFI_REQUEST_NONE;
	}/* if(ESP_GetRequestState() == (uint8_t)RESPONSE_RECEIVED) */
}
