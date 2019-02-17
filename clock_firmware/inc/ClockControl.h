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

#ifndef _CLOCKCONTROL_H_
#define _CLOCKCONTROL_H_

/*
 * This module is responsible for provide control functionality like load clockstate
 * structure from FRAM memory, change clock frequency, initialize rtc clock and init
 * touch screen calibration.
 */

#include <stdbool.h>
#include <stdint.h>
#include "FRAM_Driver.h"
#include "GUI_Clock.h"
#include "LCD.h"
#include "TouchPanel.h"
#include "chip.h"

void ClockChangeFrequency(void);
bool ClockStateLoader(uint16_t address);
void ClockInitTouchScreen(void);
void ClockSleep(uint32_t time);
void ClockInitRtc(void);

#endif /* _CLOCKCONTROL_H_ */
