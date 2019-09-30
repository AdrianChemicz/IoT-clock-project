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

#include "TemperatureSensor.h"
#include "I2C_Driver.h"
#include "GPIO_Driver.h"

static uint8_t activeState;
static uint8_t i2cStartMeasurmentCommand = 0xF3;
static uint8_t i2cReadMeasurmentValue = 0xE0;

void TemperatureSensor_Init(void)
{
	//Initialize GPIO used to control I2C transceiver
	GPIO_Direction(TEMP_GPIO_PORT_FIRST_SENSOR, TEMP_GPIO_PIN_FIRST_SENSOR, GPIO_DIR_OUTPUT);
	GPIO_Direction(TEMP_GPIO_PORT_SECOND_SENSOR, TEMP_GPIO_PIN_SECOND_SENSOR, GPIO_DIR_OUTPUT);
	GPIO_Direction(TEMP_GPIO_PORT_THIRD_SENSOR, TEMP_GPIO_PIN_THIRD_SENSOR, GPIO_DIR_OUTPUT);

	//deactivate all transceivers
	GPIO_SetState(TEMP_GPIO_PORT_FIRST_SENSOR, TEMP_GPIO_PIN_FIRST_SENSOR, false);
	GPIO_SetState(TEMP_GPIO_PORT_SECOND_SENSOR, TEMP_GPIO_PIN_SECOND_SENSOR, false);
	GPIO_SetState(TEMP_GPIO_PORT_THIRD_SENSOR, TEMP_GPIO_PIN_THIRD_SENSOR, false);
}

void TemperatureSensor_StartMeasurement(void)
{
	activeState = 1;
	TemperatureSensor_Process();
}

TempStatus TemperatureSensor_CheckMeasurementStatus(void)
{
	TempStatus tempStatusTmp = I2C_WaitingForRequest;

	switch (activeState)
	{
	case 0:
		tempStatusTmp = I2C_WaitingForRequest;
		break;

	case 1:
	case 2:
	case 3:
	case 4:
		tempStatusTmp = I2C_RequestExecuting;
		break;

	case 5:
		tempStatusTmp = I2C_DataIsReady;
		break;
	}

	return tempStatusTmp;
}

void TemperatureSensor_Process(void)
{
	switch (activeState)
	{
	case 0:
		break;
	case 1:
		I2C_SendReadData(I2C_PORT_NUMBER, SENSOR_I2C_ADDRESS, &i2cStartMeasurmentCommand, 1, 0);
		activeState = 2;
		break;
	case 2:
		if (I2C_CheckStatus(I2C_PORT_NUMBER) == I2C_WaitingForData)
			activeState = 3;
		break;
	case 3:
		I2C_SendReadData(I2C_PORT_NUMBER, SENSOR_I2C_ADDRESS, &i2cReadMeasurmentValue, 1, 2);
		activeState = 4;
		break;
	case 4:
		if (I2C_CheckStatus(I2C_PORT_NUMBER) == I2C_WaitingForData)
			activeState = 5;
		break;
	case 5:
		//data is ready for read
		break;
	}
}

bool TemperatureSensor_CheckSensorStatus(void)
{
	return true;
}

uint16_t TemperatureSensor_ReturnTemperature(void)
{
	uint8_t *data = I2C_PointerToInternalReadBuffer(I2C_PORT_NUMBER);
	activeState = 0;

	uint32_t rawTempValueInt = ((uint32_t)((data[1]) | (data[0] << 8)));

	if(rawTempValueInt == 0xFFFF)//situation when sensor was damaged or unavailable
	{
		return (uint16_t)rawTempValueInt;
	}
	else
	{
		rawTempValueInt *= 10;
		uint32_t temperatureInt = ((1757 * rawTempValueInt) / 655360);
		return (uint16_t)temperatureInt;
	}
}

void TemperatureSensor_ChoseSensor(uint8_t sensorNumber)
{
	switch(sensorNumber)
	{
	case 1:
		GPIO_SetState(TEMP_GPIO_PORT_FIRST_SENSOR, TEMP_GPIO_PIN_FIRST_SENSOR, true);
		GPIO_SetState(TEMP_GPIO_PORT_SECOND_SENSOR, TEMP_GPIO_PIN_SECOND_SENSOR, false);
		GPIO_SetState(TEMP_GPIO_PORT_THIRD_SENSOR, TEMP_GPIO_PIN_THIRD_SENSOR, false);

		break;

	case 2:
		GPIO_SetState(TEMP_GPIO_PORT_FIRST_SENSOR, TEMP_GPIO_PIN_FIRST_SENSOR, false);
		GPIO_SetState(TEMP_GPIO_PORT_SECOND_SENSOR, TEMP_GPIO_PIN_SECOND_SENSOR, true);
		GPIO_SetState(TEMP_GPIO_PORT_THIRD_SENSOR, TEMP_GPIO_PIN_THIRD_SENSOR, false);

		break;

	case 3:
		GPIO_SetState(TEMP_GPIO_PORT_FIRST_SENSOR, TEMP_GPIO_PIN_FIRST_SENSOR, false);
		GPIO_SetState(TEMP_GPIO_PORT_SECOND_SENSOR, TEMP_GPIO_PIN_SECOND_SENSOR, false);
		GPIO_SetState(TEMP_GPIO_PORT_THIRD_SENSOR, TEMP_GPIO_PIN_THIRD_SENSOR, true);

		break;

	default:

		break;
	}
}
