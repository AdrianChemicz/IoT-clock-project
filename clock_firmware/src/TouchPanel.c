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

#include "TouchPanel.h"

static bool pendingConversion;
static uint8_t portNumber;
static uint16_t xRawValue;
static uint16_t yRawValue;
//values multiply 10 times
static  uint16_t xStep;
static uint16_t yStep;
static uint16_t x0CalibrationRawValue;
static uint16_t y0CalibrationRawValue;
static uint16_t xDataTable[MAX_NUMBER_OF_SAMPLES];
static uint16_t yDataTable[MAX_NUMBER_OF_SAMPLES];

static uint16_t TouchPanel_FilterAlgorithm(uint16_t *dataTable)
{
	uint8_t valueIndex = 0;
	uint8_t matchCounter = 0;
	uint32_t filterResult = 0;
	uint8_t numOfValidSample = 0;

	//find value which the best fit to border
	for (uint8_t i = 0; i < MAX_NUMBER_OF_SAMPLES; i++)
	{
		uint8_t matchCounterTmp = 0;

		for (uint8_t j = 0; j < MAX_NUMBER_OF_SAMPLES; j++)
		{
			//if value is inside range
			if ((dataTable[j]>(dataTable[i] - VALID_VALUE_RANGE))
				&& (dataTable[j] < (dataTable[i] + VALID_VALUE_RANGE)))
			{
				matchCounterTmp++;
			}
		}

		if (matchCounterTmp > matchCounter)
		{
			matchCounter = matchCounterTmp;
			valueIndex = i;
		}
	}

	//count average value from valid values
	for (uint8_t i = 0; i < MAX_NUMBER_OF_SAMPLES; i++)
	{
		if ((dataTable[i]>(dataTable[valueIndex] - VALID_VALUE_RANGE))
			&& (dataTable[i] < (dataTable[valueIndex] + VALID_VALUE_RANGE)))
		{
			numOfValidSample++;
			filterResult += dataTable[i];
		}
	}
	
	filterResult /= (uint32_t)numOfValidSample;
	return (uint32_t)filterResult;
}

static bool TouchPanel_FilterConversion(bool irqPinState)
{
	static uint8_t conversionCounter = 0;
	
	if (irqPinState == false)
	{
		conversionCounter = 0;
	}

	//fill buffers using data from conversion
	xDataTable[conversionCounter] = xRawValue;
	yDataTable[conversionCounter] = yRawValue;
	conversionCounter++;

	//if appropriate number of sample was gathared then start process
	if (conversionCounter >= MAX_NUMBER_OF_SAMPLES)
	{
		//filtering all values
		xRawValue = TouchPanel_FilterAlgorithm(xDataTable);
		yRawValue = TouchPanel_FilterAlgorithm(yDataTable);

		//clear conversion counter
		conversionCounter = 0;

		return true;
	}
	else
	{
		return false;
	}
}

static void TouchPanel_StartConversion()
{
	SPI_PutByteToTransmitter(portNumber, X_SPI_COMMAND);
	SPI_PutByteToTransmitter(portNumber, 0);
	SPI_PutByteToTransmitter(portNumber, 0);
	SPI_PutByteToTransmitter(portNumber, Y_SPI_COMMAND);
	SPI_PutByteToTransmitter(portNumber, 0);
	SPI_PutByteToTransmitter(portNumber, 0);
}

static void TouchPanel_ReadConversionResult()
{
	uint8_t spiTmpReadValue;
	//X axis
	(void)SPI_ReadByteFromTrasmitter(portNumber);
	spiTmpReadValue = SPI_ReadByteFromTrasmitter(portNumber);
	xRawValue = spiTmpReadValue << 8;
	spiTmpReadValue = SPI_ReadByteFromTrasmitter(portNumber);
	xRawValue |= spiTmpReadValue;
	xRawValue >>= 4;
	//Y axis
	(void)SPI_ReadByteFromTrasmitter(portNumber);
	spiTmpReadValue = SPI_ReadByteFromTrasmitter(portNumber);
	yRawValue = spiTmpReadValue << 8;
	spiTmpReadValue = SPI_ReadByteFromTrasmitter(portNumber);
	yRawValue |= spiTmpReadValue;
	yRawValue >>= 4;
}

void TouchPanel_Init(uint8_t port)
{
	portNumber = port;
	//configure SPI
	SPI_DriverInit(portNumber, SPI_CLK_IDLE_LOW, SPI_CLK_LEADING);

	//configure GPIO
	//This pin is used as TP_CS
	GPIO_Direction(TOUCH_PANEL_PIN_CS_GPIO_PORT, TOUCH_PANEL_PIN_CS_GPIO_PIN, GPIO_DIR_OUTPUT);
	//set CS pin to high
	GPIO_SetState(TOUCH_PANEL_PIN_CS_GPIO_PORT, TOUCH_PANEL_PIN_CS_GPIO_PIN, true);

	//This pin is used as TP_IRQ
	GPIO_Direction(TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT, TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN, GPIO_DIR_INPUT);
	
	//set IOCONFILTCLKDIV0 clock divider responsible for glitch filter
	LPC_SYSCON->IOCONCLKDIV[IOCONFILTCLKDIV0_INDEX] = 2;

	//configure GPIO pins to work with glitch GPIO filter
#if TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT == 0
	LPC_IOCON->PIO0[TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN] |= (0<<13)|(3<<11);
#elif TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT == 1
	LPC_IOCON->PIO1[TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN] |= (0<<13)|(3<<11);
#elif TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT == 2
	//TODO: code must be added there

#else

#endif

	//init variables
	pendingConversion = false;
}

bool TouchPanel_Process(void)
{
#if SHARED_SPI_TOUCH
	//Execute when pin PENIRQ on lcd is on low state(screen was pressed) and SPI is not used
	if ((GPIO_GetState(TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT, TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN) == false)
		&& ((ClockState.sharedSpiState == NOT_USED)
		|| (ClockState.sharedSpiState == TOUCH_SCREEN_USAGE)))
#else
		//Execute when pin PENIRQ on lcd is on low state(screen was pressed)
		if (GPIO_GetState(TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT, TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN) == false)
#endif
	{
		//Execute when conversion was started
		if (pendingConversion)
		{
			//conversion is not ready
			if(SPI_CheckBusyFlag(portNumber) == true)
			{
				return false;
			}
			else//conversion is ready
			{
				//read data from SPI
				TouchPanel_ReadConversionResult();

				//start new conversion
				TouchPanel_StartConversion();

				//filtering ADC conversion
				return TouchPanel_FilterConversion(true);
			}
		}
		else//Execute when conversion wasn't started
		{
#if SHARED_SPI_TOUCH
			ClockState.sharedSpiState = TOUCH_SCREEN_USAGE;
#endif
			//set CS on low state
			GPIO_SetState(TOUCH_PANEL_PIN_CS_GPIO_PORT, TOUCH_PANEL_PIN_CS_GPIO_PIN, false);

			//start conversion
			TouchPanel_StartConversion();

			pendingConversion = true;

			return false;
		}
	}
	else//Execute when pin PENIRQ on lcd is on high state(screen wasn't pressed)
	{
		if (pendingConversion)
		{
			//conversion is not ready
			if (SPI_CheckBusyFlag(portNumber) == true)
			{
				return false;
			}
			else//conversion is ready
			{
				//read data from SPI
				TouchPanel_ReadConversionResult();

				//set CS pin to high
				GPIO_SetState(TOUCH_PANEL_PIN_CS_GPIO_PORT, TOUCH_PANEL_PIN_CS_GPIO_PIN, true);

#if SHARED_SPI_TOUCH
				ClockState.sharedSpiState = NOT_USED;
#endif
				pendingConversion = false;

				return TouchPanel_FilterConversion(false);
			}
		}
		else
		{
			return false;
		}
	}

	return false;
}

uint16_t TouchPanel_ReturnRawX(void)
{
	return xRawValue;
}

uint16_t TouchPanel_ReturnRawY(void)
{
	return yRawValue;
}

void TouchPanel_SetCalibrationParameter(uint16_t rawX1, uint16_t rawX2, uint16_t rawY1, uint16_t rawY2)
{
	uint16_t x1CalibrationRawValue = rawX1 * 10;
	uint16_t x2CalibrationRawValue = rawX2 * 10;
	uint16_t y1CalibrationRawValue = rawY1 * 10;
	uint16_t y2CalibrationRawValue = rawY2 * 10;

	//calculate x0
	xStep = (x2CalibrationRawValue - x1CalibrationRawValue) / (X2_CALIBRATION_POINT_POSITION - X1_CALIBRATION_POINT_POSITION);
	x0CalibrationRawValue = x1CalibrationRawValue - (xStep * X1_CALIBRATION_POINT_POSITION);

	//calculate y0
#if ROTATE_TOUCH
	yStep = (y2CalibrationRawValue - y1CalibrationRawValue) / (Y1_CALIBRATION_POINT_POSITION - Y2_CALIBRATION_POINT_POSITION);
	y0CalibrationRawValue = y1CalibrationRawValue - (yStep * Y2_CALIBRATION_POINT_POSITION);
#else
	yStep = (y2CalibrationRawValue - y1CalibrationRawValue) / (Y2_CALIBRATION_POINT_POSITION - Y1_CALIBRATION_POINT_POSITION);
	y0CalibrationRawValue = y1CalibrationRawValue - (yStep * Y1_CALIBRATION_POINT_POSITION);
#endif
}

uint16_t TouchPanel_CalculatePixelX(uint16_t rawX)
{
#if ROTATE_TOUCH
	return (((rawX * 10) - x0CalibrationRawValue) / xStep);
#else
	return 320 - (((rawX * 10) - x0CalibrationRawValue) / xStep);
#endif
}

uint16_t TouchPanel_CalculatePixelY(uint16_t rawY)
{
#if ROTATE_TOUCH
	return (((rawY * 10) - y0CalibrationRawValue) / yStep);
#else
	return 240 - (((rawY * 10) - y0CalibrationRawValue) / yStep);
#endif
}

//return true if LCD is touched
bool TouchPanel_TouchState(void)
{
	return !GPIO_GetState(TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT, TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN);
}
