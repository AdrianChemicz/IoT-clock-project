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

#ifndef _TOUCHPANEL_H_
#define _TOUCHPANEL_H_

#include <stdint.h>
#include <stdbool.h>
#include "GPIO_Driver.h"
#include "SPI_Driver.h"

/*
 * This module handle ADS7843(Touch screen controller) or compatible chips. To communicate
 * with module SPI port is used. SPI CS pin isn't provided by HW because SPI is shared with FRAM
 * module. Touch panel SPI CS pin was set in defines. Module also require access to Touch panel
 * IRQ pin which is in low state when is pressed this pin also is in define. SPI initialization
 * exist in this module so separate initialization isn't necessary. This module support rotation
 * of converted data which can be set via change define "ROTATE_TOUCH". Filtration algorithm was
 * added to module. Calibration point position is stored in defines.
 * This module work without any block function like sleep.
 *
 * Simple example code to read data from touch panel was added below:
 *
 *	uint16_t x, y;
 *
 *	TouchPanel_Init(1);
 *
 *	//code to get raw data
 *	if (TouchPanel_Process())
 *	{
 *		x = TouchPanel_ReturnRawX();
 *		y = TouchPanel_ReturnRawY();
 *	}
 *
 *	//code to get pixel position
 *
 *	//calibrate panel with hardcoded data - those data should be gathered from real calibration
 *	TouchPanel_SetCalibrationParameter(20, 1200, 20, 900);
 *
 *	if (TouchPanel_Process())
 *	{
 *		uint16_t xPixelPosition = TouchPanel_CalculatePixelX(TouchPanel_ReturnRawX());
 *		uint16_t yPixelPosition = TouchPanel_CalculatePixelY(TouchPanel_ReturnRawY());
 *	}
 */

#define SHARED_SPI_TOUCH 1

#if SHARED_SPI_TOUCH //include module which contain information about shared SPI usage flag
#include "GUI_Clock.h"
#endif

#define ROTATE_TOUCH 			1//Rotate about 180 degres
#define X_SPI_COMMAND 		(1 << 7) | (1 << 4) | (0 << 3) | (0 << 2)
#define Y_SPI_COMMAND 		(1 << 7) | (5 << 4) | (0 << 3) | (0 << 2)
#define MAX_NUMBER_OF_SAMPLES 				 7
#define X1_CALIBRATION_POINT_POSITION 		20
#define X2_CALIBRATION_POINT_POSITION 		300
#define CROSHAIR_RADIUS 					 6
#define VALID_VALUE_RANGE 					20
#define TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT 	 1
#define TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN  	23
#define TOUCH_PANEL_PIN_CS_GPIO_PORT 		 2
#define TOUCH_PANEL_PIN_CS_GPIO_PIN  		 2
#define IOCONFILTCLKDIV0_INDEX 				 6

#if ROTATE_TOUCH
#define Y1_CALIBRATION_POINT_POSITION 		220
#define Y2_CALIBRATION_POINT_POSITION 		20
#else
#define Y1_CALIBRATION_POINT_POSITION 		20
#define Y2_CALIBRATION_POINT_POSITION 		220
#endif

#ifdef __cplusplus
extern "C" {
#endif

	void TouchPanel_Init(uint8_t port);
	bool TouchPanel_Process(void);
	uint16_t TouchPanel_ReturnRawX(void);
	uint16_t TouchPanel_ReturnRawY(void);
	void TouchPanel_SetCalibrationParameter(uint16_t rawX1, uint16_t rawX2, uint16_t rawY1, uint16_t rawY2);
	uint16_t TouchPanel_CalculatePixelX(uint16_t rawX);
	uint16_t TouchPanel_CalculatePixelY(uint16_t rawY);
	bool TouchPanel_TouchState(void);

#ifdef __cplusplus
}
#endif

#endif /* _TOUCHPANEL_H_ */
