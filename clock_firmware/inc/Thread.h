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

#ifndef _THREAD_H_
#define _THREAD_H_

/*
 * This module provide thread functionality which is used to execute not blocking
 * functions responsible for operations on FRAM memory, factory reset button, allarms
 * and set refresh GUI flag.
 */

#include "chip.h"
#include "GUI_Clock.h"
#include "GPIO_Driver.h"
#include "FRAM_Driver.h"
#include "TemperatureSensor.h"
#include "TouchPanel.h"
#include "BuzzerControl.h"
#include "WIFI_InteractionLayer.h"
#include <core_cm0plus.h>
#include <crc_11u6x.h>
#include <error.h>
#include <stdbool.h>
#include <stdint.h>
#include <syscon_11u6x.h>
#include <timer_11u6x.h>

#define ONE_SECONDS 						50
#define RESET_BUTTON_GPIO_PORT 				2
#define RESET_BUTTON_GPIO_PIN 				4
#define CLEAR_BLOCK_SIZE 					100
#define MAX_INDEX (FRAM_MEASUREMENT_DATA_BEGIN/CLEAR_BLOCK_SIZE)
#define TRANSACTION_NOT_DEFINED 			0
#define TRANSACTION_COPY_TO_BACKUP 			1
#define TRANSACTION_COPY_TO_DEDICATED_AREA 	2
#define SEARCH_RADIUS 						6
#define LENGHT_OF_SOUND_ALARM_TABLE 		20
#define LENGHT_OF_INCREMENT_SOUND_STEP 		3

void Thread_Init(void);
void Thread_Call(void);

#endif /* _THREAD_H_ */
