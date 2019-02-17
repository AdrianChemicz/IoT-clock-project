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

#include "ClockControl.h"

void ClockSleep(uint32_t time)
{
#if 0 //good value for 12MHz // for 1ms wait is equal 1,89ms tested under logic analyzer
	for (volatile uint32_t count = 0; count < (uint32_t)(time<<11);)
#endif
#if 1 //good value for 48MHz
	for (volatile uint32_t count = 0; count < (uint32_t)(time<<13);)
#endif
	{
		 count++;
	}
}

void ClockChangeFrequency(void)
{
	/* Select PLL Input      1=System oscillator. Crystal Oscillator (SYSOSC)
	   0x0 IRC */
	LPC_SYSCON->SYSPLLCLKSEL = 0;

	LPC_SYSCON->SYSPLLCLKUEN  = 0x01;               /* Update Clock Source      */
	LPC_SYSCON->SYSPLLCLKUEN  = 0x00;               /* Toggle Update Register   */
	LPC_SYSCON->SYSPLLCLKUEN  = 0x01;

	/* Wait Until Updated       */
	while (!(LPC_SYSCON->SYSPLLCLKUEN & 0x01));

	/* Power-up SYSPLL          */
	LPC_SYSCON->SYSPLLCTRL = 0x23;
	/* Power-up WDT Clock       */
	LPC_SYSCON->PDRUNCFG &= ~(1 << 7);

	/* Wait Until PLL Locked    */
	while (!(LPC_SYSCON->SYSPLLSTAT & 0x01));

	LPC_SYSCON->MAINCLKSEL    = 3;     /* Select PLL Clock Output  */
	LPC_SYSCON->MAINCLKUEN    = 0x01;               /* Update MCLK Clock Source */
	LPC_SYSCON->MAINCLKUEN    = 0x00;               /* Toggle Update Register   */
	LPC_SYSCON->MAINCLKUEN    = 0x01;

	/* Wait Until Updated       */
	while (!(LPC_SYSCON->MAINCLKUEN & 0x01));

	LPC_SYSCON->SYSAHBCLKDIV  = 1;
}

bool ClockStateLoader(uint16_t address)
{
	//read from FRAM parameters
	FRAM_Read(address, sizeof(ClockState), (uint8_t*)&ClockState);
	for(int i=0;i<100000;i++)
	{
		if(FRAM_Process())
		{
			i = 100000;
		}
		ClockSleep(1);
	}

	//if ClockState contain only zeros treat as invalid
	bool emptyStructure = true;
	for(uint16_t i =0;i<(uint16_t)(sizeof(ClockState));i++)
	{
		uint8_t* clockStateDataPointer = (uint8_t*)(&ClockState);
		if(clockStateDataPointer[i] != 0)
		{
			emptyStructure = false;
			break;
		}
	}

	if(emptyStructure == true)
		return false;

	//calculate and compare checksums
	uint16_t checkSumValue = Chip_CRC_CRC16((uint16_t*)&ClockState, (offsetof(ClockStateType, CRC16Value)/2));

	if(checkSumValue == ClockState.CRC16Value)
		return true;
	else
		return false;
}

void ClockInitTouchScreen(void)
{
	uint16_t x1 = 0, x2 = 0, y1 = 0, y2 = 0;

	LCD_FillFrame(0, 0, 319, 239, 0xFFFF);

	//get first calibration point
	for (int i = 0; i < (1 + (CROSHAIR_RADIUS * 2)); i++)
	{
		LCD_SetPixel((X2_CALIBRATION_POINT_POSITION - CROSHAIR_RADIUS) + i, Y1_CALIBRATION_POINT_POSITION, 0);
		LCD_SetPixel(X2_CALIBRATION_POINT_POSITION, ((Y1_CALIBRATION_POINT_POSITION - CROSHAIR_RADIUS) + i), 0);
	}

	for (bool flag = true; flag;)
	{
		if (TouchPanel_Process())
		{
			x1 = TouchPanel_ReturnRawX();
			y1 = TouchPanel_ReturnRawY();

			flag = false;
		}
		ClockSleep(20);
	}

	ClockSleep(2000);

	for (int i = 0; i < (1 + (CROSHAIR_RADIUS * 2)); i++)
	{
		LCD_SetPixel((X1_CALIBRATION_POINT_POSITION - CROSHAIR_RADIUS) + i, Y1_CALIBRATION_POINT_POSITION, 0);
		LCD_SetPixel(X1_CALIBRATION_POINT_POSITION, ((Y1_CALIBRATION_POINT_POSITION - CROSHAIR_RADIUS) + i), 0);
	}

	for (bool flag = true; flag;)
	{
		if (TouchPanel_Process())
		{
			x2 = TouchPanel_ReturnRawX();
			y1 = TouchPanel_ReturnRawY();

			flag = false;
		}
		ClockSleep(20);
	}

	ClockSleep(2000);

	for (int i = 0; i < (1 + (CROSHAIR_RADIUS * 2)); i++)
	{
		LCD_SetPixel((X1_CALIBRATION_POINT_POSITION - CROSHAIR_RADIUS) + i, Y2_CALIBRATION_POINT_POSITION, 0);
		LCD_SetPixel(X1_CALIBRATION_POINT_POSITION, ((Y2_CALIBRATION_POINT_POSITION - CROSHAIR_RADIUS) + i), 0);
	}

	for (bool flag = true; flag;)
	{
		if (TouchPanel_Process())
		{
			x2 = TouchPanel_ReturnRawX();
			y2 = TouchPanel_ReturnRawY();

			flag = false;
		}
		ClockSleep(20);
	}

	ClockState.x1RawCalibrationValue = x1;
	ClockState.x2RawCalibrationValue = x2;
	ClockState.y1RawCalibrationValue = y1;
	ClockState.y2RawCalibrationValue = y2;
}

void ClockInitRtc(void)
{
	Chip_Clock_EnableRTCOsc();

	Chip_RTC_Init(LPC_RTC);

	Chip_RTC_Reset(LPC_RTC);

	Chip_RTC_Disable(LPC_RTC);

	Chip_RTC_SetCount(LPC_RTC, 0);

	Chip_RTC_SetAlarm(LPC_RTC, 1);

	Chip_RTC_Enable(LPC_RTC);

	Chip_RTC_ClearStatus(LPC_RTC, (RTC_CTRL_OFD | RTC_CTRL_ALARM1HZ | RTC_CTRL_WAKE1KHZ));

	NVIC_EnableIRQ(RTC_IRQn);

	Chip_RTC_EnableOptions(LPC_RTC, RTC_CTRL_ALARM1HZ);
}

void RTC_IRQHandler(void)
{
	GUI_IncrementSecond();

	/* Clear latched RTC status */
	Chip_RTC_EnableOptions(LPC_RTC, RTC_CTRL_ALARM1HZ);

	Chip_RTC_Disable(LPC_RTC);
	Chip_RTC_SetCount(LPC_RTC, 0);
	Chip_RTC_SetAlarm(LPC_RTC, 1);
	Chip_RTC_Enable(LPC_RTC);
}
