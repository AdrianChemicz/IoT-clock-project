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

#include "BuzzerControl.h"

static const uint16_t octaveTable[4] = {50, 25, 16, 2};

void Buzzer_Init(void)
{
	//configure PWM

	//turn on options responsible for change GPIO purpose like UART, ADC or SPI
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);

	//connect CT32B1 to AHB
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<10);

	//configure pin nTRST/PIO0_14/ADC_6/CT32B1_MAT1/U1_TXD to work as PWM
	LPC_IOCON->PIO0[14] |= 3;

	//configure period to be compatible with buzzer (4500Hz)
#if 0
	//configure prescaler. Value appropriate for 12 MHz IRC
	LPC_TIMER32_1->PR = 26;
#else
	//configure prescaler. Value appropriate for 48 MHz PPL
	LPC_TIMER32_1->PR = 104;
#endif
	//set value which will be used as max of counter
	LPC_TIMER32_1->MR[3] = 100;

	//this register will hold length of pulse
	LPC_TIMER32_1->MR[1] = 50;

	//Reset on MR3(set MR3R bit)
	LPC_TIMER32_1->MCR = (1<<10);

	//enable PWM mode for channel1
	LPC_TIMER32_1->PWMC = 0x2;

	//Reset Timer
	LPC_TIMER32_1->TCR = 0x2;
}

void Buzzer_TurnOn(void)
{
	//Reset Timer
	LPC_TIMER32_1->TCR = 0x2;

	//Enable timer (set CEN bit)
	LPC_TIMER32_1->TCR = 0x1;
}

void Buzzer_TurnOff(void)
{
	//Reset Timer
	LPC_TIMER32_1->TCR = 0x2;
}

void Buzzer_SetBuzzerPwmCounterRegister(uint16_t value)
{
	//set value which will be used as max of counter
	LPC_TIMER32_1->MR[3] = value*2;

	//this register will hold length of pulse
	LPC_TIMER32_1->MR[1] = value;
}

void Buzzer_SetOctave(uint8_t octaveNumber)
{
	Buzzer_SetBuzzerPwmCounterRegister(octaveTable[octaveNumber]);
}

