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

#ifndef _TEMPERATURE_SENSOR_H_
#define _TEMPERATURE_SENSOR_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * This module handle Si7021-A20 (I2C humidity and temperature sensor). Other sensors wasn't supported.
 * On clock PCB exist three PCA9507 I2C bus expander which allow to select one of three sensors via GPIO.
 * GPIO used to select sensor is connected to EN pin in PCA9507 chip. Number of GPIO port and I2C was set
 * in defines in this file. I2C port must be initialized separately.
 * This module work without any block function like sleep.
 *
 * Simple example code to receive data from sensor:
 *
 * 	//init bus for thermal sensors
 *	I2C_DriverInit(0);
 *
 *	//init GPIO for manage I2C transceiver
 *	TemperatureSensor_Init();
 *
 *	TemperatureSensor_ChoseSensor(1);
 *
 *	TemperatureSensor_StartMeasurement();
 *	volatile uint16_t temperature;
 *	for (;;)
 *	{
 *		TemperatureSensor_Process();
 *		if (TemperatureSensor_CheckMeasurementStatus() == I2C_DataIsReady)
 *		{
 *			temperature = TemperatureSensor_ReturnTemperature();
 *		}
 *
 *		if (TemperatureSensor_CheckMeasurementStatus() == I2C_WaitingForRequest)
 *		{
 *			TemperatureSensor_StartMeasurement();
 *		}
 *	}
 */

#define SENSOR_I2C_ADDRESS 		0x80
#define I2C_PORT_NUMBER 		0

#define TEMP_GPIO_PORT_FIRST_SENSOR 	 2
#define TEMP_GPIO_PIN_FIRST_SENSOR 		 5
#define TEMP_GPIO_PORT_SECOND_SENSOR 	 0
#define TEMP_GPIO_PIN_SECOND_SENSOR 	20
#define TEMP_GPIO_PORT_THIRD_SENSOR 	 0
#define TEMP_GPIO_PIN_THIRD_SENSOR 		 2

#define CN6_TEMP_INSIDE 	1
#define CN5_TEMP_OUTSIDE 	2
#define CN4_TEMP_FURNACE 	3

#define TEMPERATURE_OFFSET_FROM_ZERO 	468

#ifdef __cplusplus
extern "C" {
#endif
	typedef enum TempStatus
	{
		I2C_WAITING_FOR_REQUEST,
		I2C_REQUEST_EXECUTING,
		I2C_DATA_IS_READY,
	}TempStatus;

	void TemperatureSensor_Init(void);
	void TemperatureSensor_StartMeasurement(void);
	TempStatus TemperatureSensor_CheckMeasurementStatus(void);
	void TemperatureSensor_Process(void);
	bool TemperatureSensor_CheckSensorStatus(void);
	uint16_t TemperatureSensor_ReturnTemperature(void);
	void TemperatureSensor_ChoseSensor(uint8_t sensorNumber);

#ifdef __cplusplus
}
#endif

#endif  /* _TEMPERATURE_SENSOR_H_ */
