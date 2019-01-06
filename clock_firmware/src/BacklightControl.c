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

#include "BacklightControl.h"

void Backlight_Init(void)
{
	/**********************************
	*	configure PWM
	***********************************/
	//turn on options responsible for change GPIO purpose like UART, ADC or SPI
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<16);

	//connect CT32B0 to AHB
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<9);

	//configure pin PIO2_3/U3_RXD/CT32B0_MAT1 to work as PWM
	LPC_IOCON->PIO2B[1] |= 2;
#if 0
	//configure prescaler. Value appropriate for 12 MHz IRC
	LPC_TIMER32_0->PR = 4;
#else
	//configure prescaler. Value appropriate for 48 MHz PPL
	LPC_TIMER32_0->PR = 16;
#endif
	//set value which will be used as max of counter
	LPC_TIMER32_0->MR[3] = 100;

	//this register will hold length of pulse
	LPC_TIMER32_0->MR[1] = 1;

	//Reset on MR3(set MR3R bit)
	LPC_TIMER32_0->MCR = (1<<10);

	//enable PWM mode for channel1
	LPC_TIMER32_0->PWMC = 0x2;

	//Reset Timer
	LPC_TIMER32_0->TCR = 0x2;

	//Enable timer (set CEN bit)
	LPC_TIMER32_0->TCR = 0x1;
}

void Backlight_SetBrightness(uint8_t value)
{
	if(value >= 100)
		value = 99;

	if(value == 0)
		value = 1;

	//Reset Timer
	LPC_TIMER32_0->TCR = 0x2;

	LPC_TIMER32_0->MR[1] = (100 - value);

	//Enable timer (set CEN bit)
	LPC_TIMER32_0->TCR = 0x1;
}
