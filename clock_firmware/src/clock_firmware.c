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

#ifdef __USE_CMSIS
#include "chip.h"
#endif

#include "GPIO_Driver.h"
#include "SPI_Driver.h"
#include "UART_Driver.h"
#include "I2C_Driver.h"
#include "TouchPanel.h"
#include "GUI_Clock.h"
#include "TemperatureSensor.h"
#include "BacklightControl.h"
#include "BuzzerControl.h"
#include "FRAM_Driver.h"
#include "Thread.h"
#include "ClockControl.h"
#include "ESP_Layer.h"
#include "LCD.h"

#include <stdint.h>
#include <cr_section_macros.h>

int main(void)
{
	/**********************************
	*	initialize section
	***********************************/
	ClockChangeFrequency();

	//init touch mode
	TouchPanel_Init(1);

	//init GPIO port
	GPIO_Init();

	//init real LCD
	LCD_Init(0);

	FRAM_Init(1);

	//init backlight parameter
	Backlight_Init();

	Buzzer_Init();

	Chip_CRC_Init();

	//init bus for thermal sensors
	I2C_DriverInit(0);

	//init GPIO for manage I2C transceiver
	TemperatureSensor_Init();

	ESP_Init(0, 115200, 400, 3, 0);

	UART_DisableInterrupts(ESP_GetUartPortNumber());

	//search structure data in FRAM memory - two valid block is stored.
	if(ClockStateLoader(FRAM_CLOCK_STATE_FIRST_COPY))
	{

	}
	else if(ClockStateLoader(FRAM_CLOCK_STATE_SECOND_COPY))
	{

	}
	else//if all block is invalid then clear structure and init clock again
	{
		memset(&ClockState, 0, sizeof(ClockState));

		//set default brightness value
		ClockState.brightness = 80;

		//get calibration parameter for LCD
		ClockInitTouchScreen();

		//set default parameter for calendar - this parameter will be interpret as __ - __ - ____
		ClockState.day = INVALID_CALENDAR_DATE;
		ClockState.month = INVALID_CALENDAR_DATE;
		ClockState.year = INVALID_CALENDAR_DATE;

		//initialize FRAM index of all temperature sources
		ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureFramIndex = NOT_INITIALIZED_FRAM_INDEX_VALUE;
		ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureFramIndex = NOT_INITIALIZED_FRAM_INDEX_VALUE;
		ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureFramIndex = NOT_INITIALIZED_FRAM_INDEX_VALUE;
	}

	//set brightness
	Backlight_SetBrightness(ClockState.brightness);

	//calibrate LCD
	TouchPanel_SetCalibrationParameter(ClockState.x1RawCalibrationValue, ClockState.x2RawCalibrationValue,
		ClockState.y1RawCalibrationValue, ClockState.y2RawCalibrationValue);

	//clear fields which state must be checked again in new lifecycle
	ClockState.sharedSpiState = NOT_USED;
	ClockState.FramTransactionIdentifier = 0;

	//fields responsible for temperature
	ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValue = INVALID_READ_SENSOR_VALUE;
	ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValid = false;

	ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValue = INVALID_READ_SENSOR_VALUE;
	ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValid = false;

	ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue = INVALID_READ_SENSOR_VALUE;
	ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValid = false;

	ClockState.refreshTemperatureValues = false;

	//clear fields responsible for raise allarm
	ClockState.firstAlarmRaised = false;
	ClockState.secondAlarmRaised = false;
	ClockState.temperatureFurnaceAlarmRaised = false;

	//clear counter responsible for refresh GUI interface if touch wasn't detected during 10s
	ClockState.refreshGuiCounter = 0;

	ClockState.factoryResetViaGui = false;

	//clear WiFi flags and IP address
	ClockState.wifiReady = false;
	ClockState.wifiApnReceived = false;
	ClockState.wifiConnected = false;
	ClockState.wifiStartDisconnect = false;

	memset(ClockState.ipAddressAssignedToDevice, 0, IP_ADDRESS_BYTE_LENGTH);

	//init structures responsible for draw temperature
	for(int i = 0; i < READ_TEMP_FRAM_BUFFER_SIZE; i++)
	{
		ReadFramTempBufferTable[i].availabilityFlag = false;
		ReadFramTempBufferTable[i].framIndex = 0;
		ReadFramTempBufferTable[i].notExistFlag = false;

		if(i == (READ_TEMP_FRAM_BUFFER_SIZE - 1))
		{
			ReadFramTempBufferTable[i].pointerToNextElement = &ReadFramTempBufferTable[0];
			ReadFramTempBufferTable[i].pointerToPreviousElement = &ReadFramTempBufferTable[i - 1];
		}
		else if(i == 0)
		{
			ReadFramTempBufferTable[i].pointerToNextElement = &ReadFramTempBufferTable[i + 1];
			ReadFramTempBufferTable[i].pointerToPreviousElement = &ReadFramTempBufferTable[(READ_TEMP_FRAM_BUFFER_SIZE-1)];
		}
		else
		{
			ReadFramTempBufferTable[i].pointerToNextElement = &ReadFramTempBufferTable[i + 1];
			ReadFramTempBufferTable[i].pointerToPreviousElement = &ReadFramTempBufferTable[i - 1];
		}
	}

	GUI_ClockInit();

	Thread_Init();

	ClockInitRtc();

	/**********************************
	*	main loop
	***********************************/
	for(;;)
	{
		static bool touchPanelState = false;

		if(TouchPanel_Process())
		{
			touchPanelState = true;
			uint16_t xPixelPosition = TouchPanel_CalculatePixelX(TouchPanel_ReturnRawX());
			uint16_t yPixelPosition = TouchPanel_CalculatePixelY(TouchPanel_ReturnRawY());

#if 1		//draw point after touch for debug purpose
			UG_DrawFrame(xPixelPosition - 2, yPixelPosition - 2, xPixelPosition + 2, yPixelPosition + 2, 0);
#endif
			UG_TouchUpdate(TouchPanel_CalculatePixelX(TouchPanel_ReturnRawX()),
				TouchPanel_CalculatePixelY(TouchPanel_ReturnRawY()), TOUCH_STATE_PRESSED);

			GUI_UpdateTime();
			UG_Update();

			ClockState.refreshGuiCounter = 0;
		}

		if(touchPanelState && !TouchPanel_TouchState())
		{
			UG_TouchUpdate(-1, -1, TOUCH_STATE_RELEASED);
			UG_Update();
			UG_Update();
			touchPanelState = false;

			ClockState.refreshGuiCounter = 0;
		}

		if(ClockState.refreshTemperatureValues == REFRESH)
		{
			GUI_UpdateTemperature();
			ClockState.refreshTemperatureValues = REFRESH_PERFORMED;
		}

		if(ClockState.refreshGuiCounter >= 5*ONE_SECONDS)
		{
			GUI_UpdateTime();

			UG_Update();
			ClockState.refreshGuiCounter = 0;
		}

		GUI_ProcessTemperatureWindow();
		GUI_ProcessAlarmAnimation();
		GUI_RefreshWifiWindow();
	}/* for(;;) */

    return 0;
}
