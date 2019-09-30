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

#ifndef _GUI_CLOCK_H_
#define _GUI_CLOCK_H_

/*
 * This module is responsible for clock GUI. Initialize all windows, call execution
 * appropriate operation triggered from GUI button and register SPI drivers in uGui
 * library. Only this module use direcly uGui library.
 */

#include <stdbool.h>
#include <stdint.h>
#include "ugui.h"
#include "ugui_config.h"
#include "image.h"
#include "ESP_Layer.h"

#define MAX_OBJECTS 17
#define MAX_OBJECTS_SETTINGS 18
#define MAX_OBJECTS_WIFI_SETTINGS 17
#define MAX_OBJECTS_KEYBOARD_WINDOW 46

#define DEFAULT_GUI_BACKGROUND_COLOR C_WHITE_SMOKE

#define INVALID_READ_SENSOR_VALUE 0xFFFF

//main window
#define PICTURE_HEIGHT 66
#define PICTURE_LENGTH 70
#define ALARM_BUTTON_BORDER_DISTANCE 6
#define LABEL_LENGTH 71
#define ALARM_BUTTON_LENGTH_MW 27
#define ALARM_BUTTON_HEIGH_MW 25

//grid parameter for main window
#define GRID_X1_MW 3
#define GRID_X2_MW 76
#define GRID_X3_MW 155
#define GRID_X4_MW 230
#define GRID_Y1_MW 10
#define GRID_Y2_MW 93
#define GRID_Y3_MW 164
#define GRID_Y1_SECOND_BUTTON_MW 37
#define GRID_Y1_THIRD_BUTTON_MW 64

//clock settings window
#define SET_TIME_BUTTON_HEIGHT 30
#define SET_TIME_BUTTON_LENGTH 50

//grid parameters for clock settings window
#define GRID_X1_CSW 50
#define GRID_X2_CSW 211
#define GRID_Y1_CSW 30
#define GRID_Y2_CSW 123

//parameters for temperature window
#define GRID_X1_TW 118
#define GRID_X2_TW 262
#define GRID_Y1_TW 165
#define GRID_Y2_TW 185
#define GRID_Y3_TW 201

#define SET_TEMPERATURE_BUTTON_HEIGHT 14
#define SET_TEMPERATURE_BUTTON_LENGTH 22

#define TEMPERATURE_GRAPH_X_START 35
#define TEMPERATURE_GRAPH_Y_START 40
#define TEMPERATURE_GRAPH_WIDH 240
#define TEMPERATURE_GRAPH_HEIGH 100

#define MAX_STEP_ON_GRAPH 15

#define NUM_OF_MEASUREMENTS_ON_LEFT_SIDE 40
#define NUM_OF_MEASUREMENTS_ON_RIGHT_SIDE 40
#define CURSOR_POSITION_ON_X_AXIS (NUM_OF_MEASUREMENTS_ON_LEFT_SIDE)
#define NUM_OF_MEASUREMENTS_IN_X_AXIS (NUM_OF_MEASUREMENTS_ON_LEFT_SIDE + 1 + NUM_OF_MEASUREMENTS_ON_RIGHT_SIDE)
#define NUM_OF_FRAM_BLOCK_IN_TABLE 3
#define X_AXIS_DISTANCE_FROM_FRAME 2
#define X_AXIS_ENTRIES_LENGTH 3
#define DRAW_THRESHOLD_TIMESTAMP 28
#define INVALID_INIT_TEMPERTAURE 0xFFF0
#define NUMBER_OF_MEASUREMENTS_IN_FILTER_TABLE 6

//parameters for settings window
#define LINE_HEIGH_SW 20
#define FIRST_LINE_BEGIN_SW 1
#define SECOND_LINE_BEGIN_SW 26
#define THIRD_LINE_BEGIN_SW 51
#define FOURTH_LINE_BEGIN_SW 76

#define CALENDAR_GRID_X1_SW 2
#define CALENDAR_GRID_X2_SW 42
#define CALENDAR_GRID_X3_SW 90
#define CALENDAR_GRID_Y1_SW 165
#define CALENDAR_GRID_Y2_SW 180
#define CALENDAR_GRID_Y3_SW 200
#define CALENDAR_SWITCH_LENGTH_SW 16
#define CALENDAR_SWITCH_HEIGH_SW 10
#define INVALID_CALENDAR_DATE 255

//parameters for WiFi settings window
#define FIRST_LINE_OF_APN_BEGIN_WSW 75
#define SECOND_LINE_OF_APN_BEGIN_WSW 100
#define THIRD_LINE_OF_APN_BEGIN_WSW 125
#define FOURTH_LINE_OF_APN_BEGIN_WSW 150
#define FIVETH_LINE_OF_APN_BEGIN_WSW 175
#define APN_LIST_LINE_HEIGH_WSW 20
#define APN_NAME_BEGIN 1
#define APN_NAME_END 190
#define APN_CONNECT_BUTTON_BEGIN 195
#define APN_CONNECT_BUTTON_END 290
#define APN_SLIDER_BEGIN 295
#define APN_SLIDER_END 310
#define NUMBER_OF_WIFI_DISPLAY_NETWORKS 5
#define TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN TXB_ID_3
#define BUTTON_WIFI_DISPLAY_NETWORK_BEGIN BTN_ID_2

//parameters for WiFi keyboard window
#define ROW_HEIGH_WKW 37
#define BUTTON_HEIGH_WKW 30
#define FIRST_ROW_OF_BUTTONS_BEGIN_WKW 65
#define SECOND_ROW_OF_BUTTONS_BEGIN_WKW (FIRST_ROW_OF_BUTTONS_BEGIN_WKW + (ROW_HEIGH_WKW * 1))
#define THIRD_ROW_OF_BUTTONS_BEGIN_WKW (FIRST_ROW_OF_BUTTONS_BEGIN_WKW + (ROW_HEIGH_WKW * 2))
#define FOURTH_ROW_OF_BUTTONS_BEGIN_WKW (FIRST_ROW_OF_BUTTONS_BEGIN_WKW + (ROW_HEIGH_WKW * 3))

#define BUTTON_COLUMN_BEGIN_WKW 7
#define BUTTON_DISTANCE_BETWEEN_COLUMN_WKW 27
#define BUTTON_COLUMN_NR1_WKW 20
#define BUTTON_COLUMN_NR2_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*1))
#define BUTTON_COLUMN_NR3_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*1))
#define BUTTON_COLUMN_NR4_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*2))
#define BUTTON_COLUMN_NR5_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*2))
#define BUTTON_COLUMN_NR6_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*3))
#define BUTTON_COLUMN_NR7_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*3))
#define BUTTON_COLUMN_NR8_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*4))
#define BUTTON_COLUMN_NR9_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*4))
#define BUTTON_COLUMN_NR10_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*5))
#define BUTTON_COLUMN_NR11_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*5))
#define BUTTON_COLUMN_NR12_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*6))
#define BUTTON_COLUMN_NR13_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*6))
#define BUTTON_COLUMN_NR14_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*7))
#define BUTTON_COLUMN_NR15_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*7))
#define BUTTON_COLUMN_NR16_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*8))
#define BUTTON_COLUMN_NR17_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*8))
#define BUTTON_COLUMN_NR18_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*9))
#define BUTTON_COLUMN_NR19_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*9))
#define BUTTON_COLUMN_NR20_WKW (BUTTON_COLUMN_BEGIN_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*10))
#define BUTTON_COLUMN_NR21_WKW (BUTTON_COLUMN_NR1_WKW + (BUTTON_DISTANCE_BETWEEN_COLUMN_WKW*10))
#define BUTTON_COLUMN_END_WKW 300
#define BUTTON_LENGTH_WKW 22

#define BUTTON_WIFI_KEYBOARD_BEGIN 				BTN_ID_1
#define SHIFT_KEY_OFFSET_WKW 					21
#define BACKSPACE_KEY_OFFSET_WKW 				31
#define LETTER_OR_SPECIAL_CHAR_KEY_OFFSET_WKW 	32
#define ENTER_KEY_OFFSET_WKW 					38
#define NUMBER_OF_KEY_ON_KEYBOARD 				39
#define MAX_APN_PASSWORD_LENGTH 				64

//FRAM unique ID names
#define FRAM_ID_NOP 						0
#define FRAM_ID_CLOCKSTATE 					1
#define FRAM_ID_FACTORY_RESET 				2
#define FRAM_ID_MOVE_TEMPERATURE_OUTSIDE 	3
#define FRAM_ID_MOVE_TEMPERATURE_INSIDE 	4
#define FRAM_ID_MOVE_TEMPERATURE_FURNACE 	5
#define FRAM_ID_READ_TEMPERATURE 			6
#define FRAM_ID_SEARCH_TEMPERATURE 			7

#define MAX_TEMP_RECORD_PER_DAY 96
#define MAX_RECORD_IN_FRAM ((0x7FFF - FRAM_MEASUREMENT_DATA_BEGIN)/(sizeof(TemperatureSingleDayRecordType)))

#define READ_TEMP_FRAM_BUFFER_PREVIOUS 2
#define READ_TEMP_FRAM_BUFFER_NEXT 1
#define READ_TEMP_FRAM_BUFFER_SIZE (READ_TEMP_FRAM_BUFFER_NEXT + 1 + READ_TEMP_FRAM_BUFFER_PREVIOUS)

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum ACTIVE_TIME_SETTINGS
	{
		CURRENT_TIME = 0,
		FIRST_ALARM = 1,
		SECOND_ALARM = 2
	}ACTIVE_TIME_SETTINGS;

	typedef enum TEMPERATURE_TYPE
	{
		OUTSIDE_TEMPERATURE = 0,
		INSIDE_TEMPERATURE = 1,
		FURNACE_TEMPERATURE = 2,
		NUM_OF_TEMPERATURE_SOURCE
	}TEMPERATURE_TYPE;

	typedef enum TYPE_OF_OPERATION
	{
		INCREMENT = 0,
		DECREMENT = 1
	}TYPE_OF_OPERATION;

	typedef enum SHARED_SPI_STATE_TYPE
	{
		NOT_USED = 0,
		FRAM_USAGE = 1,
		TOUCH_SCREEN_USAGE = 2
	}SHARED_SPI_STATE_TYPE;

	typedef enum REFRESH_GUI_TYPE
	{
		WAITING_FOR_REQUEST = 0,
		REFRESH = 1,
		REFRESH_PERFORMED = 2
	}REFRESH_GUI;

	typedef enum WIFI_GUI_STATUS_TYPE//connected to: , active, inactive, WiFi damaged
	{
		WIFI_INACTIVE = 0,
		WIFI_ACTIVE = 1,
		WIFI_CONNECTED = 2
	}WIFI_GUI_STATUS;

	typedef enum KEYBOARD_SIGNS_TYPE
	{
		SMALL_LETTER = 0,
		BIG_LETTER = 1,
		SPECIAL_CHARACTER = 2
	}KEYBOARD_SIGNS;

	typedef struct
	{
		uint8_t day;
		uint8_t month;
		uint8_t year;
	}CalendarValueInSettingsType;

	typedef struct
	{
		uint8_t source;
		uint8_t day;
		uint8_t month;
		uint8_t year;
		uint16_t temperatureValues[MAX_TEMP_RECORD_PER_DAY];
		uint16_t CRC16Value  __attribute__((aligned(32)));
	}TemperatureSingleDayRecordType;

	typedef struct
	{
		bool temperatureValid;
		bool recordTemperature;
		uint16_t temperatureValue;
		uint16_t temperatureFramIndex;
	}TemperatureStructureType;

	typedef struct
	{
		ACTIVE_TIME_SETTINGS activeTimeSettings;
		SHARED_SPI_STATE_TYPE sharedSpiState;
		uint8_t currentTimeHour;
		uint8_t currentTimeMinute;
		uint8_t currentTimeSecond;

		uint8_t day;
		uint8_t month;
		uint8_t year;

		uint8_t firstAlarmHour;
		uint8_t firstAlarmMinute;
		bool firstAlarmActive;
		bool firstAlarmRaised;

		uint8_t secondAlarmHour;
		uint8_t secondAlarmMinute;
		bool secondAlarmActive;
		bool secondAlarmRaised;

		TemperatureStructureType TemperatureSensorTable[NUM_OF_TEMPERATURE_SOURCE];

		uint16_t temperatureFurnaceAlarmThreshold;
		int8_t temperatureFurnaceOffset;
		bool temperatureFurnaceAlarmActive;
		bool temperatureFurnaceAlarmRaised;
		TEMPERATURE_TYPE temperatureTypeInWindow;

		uint8_t FramTransactionIdentifier;
		uint16_t currentFramIndex; //this value point next free index

		//refresh GUI
		REFRESH_GUI refreshTemperatureValues;
		uint16_t refreshGuiCounter;

		uint8_t brightness;
		bool factoryResetViaGui;
		uint32_t systemUpTime; //value is stored in seconds

		//piezo
		uint16_t alarmSoundAnimationStep;

		//LCD calibration parameter
		uint16_t x1RawCalibrationValue;
		uint16_t x2RawCalibrationValue;
		uint16_t y1RawCalibrationValue;
		uint16_t y2RawCalibrationValue;

		//WiFi
		bool wifiReady;
		bool wifiConnected;
		bool wifiApnReceived;
		bool wifiStartDisconnect;
		uint8_t ssidOfAssignedApn[SSID_STRING_LENGTH];
		uint8_t passwordToAssignedApn[MAX_APN_PASSWORD_LENGTH];
		uint8_t ipAddressAssignedToDevice[4];
#ifdef MICROCONTROLLER
		uint16_t CRC16Value  __attribute__((aligned(32)));
	}ClockStateType __attribute__((aligned(32)));
#else
		uint16_t CRC16Value;
}ClockStateType;
#endif

	typedef struct
	{
		//string to all widget(like TEXTBOX and BUTTON) where is necessary dynamic calculated string
		uint8_t labelClockSettingsTimeValue[8];
		uint8_t labelMainWindowClockTimeValue[8];
		uint8_t labelTemperatureOutside[8];
		uint8_t labelTemperatureInside[8];
		uint8_t labelTemperatureFurnace[8];
		uint8_t labelCalendarValue[17];

		uint8_t labelTemperatureInTitleOfWindow[40];
		uint8_t labelTemperatureFurnaceOffset[31];
		uint8_t labelTemperatureFurnaceAlarm[18];

		uint8_t labelBrightnessValue[5];
		uint8_t labelUpTime[26];
		uint8_t labelGraphStep[10];
		uint8_t labelWifiStatus[50];
		uint8_t labelAssignedIpAdress[24];
		uint8_t labelApnPassword[MAX_APN_PASSWORD_LENGTH];
	}WidgetsStringsType;

	typedef struct
	{
		bool startTemperatureTransaction;//this flag is set every 15 minutes and mean that FRAM content should be updated
		uint8_t transactionStep;/* step value which is used during store structure in FRAM. In first step
			data is loaded to backup area in second step is loaded to dedicated area pointed by FRAM index */
		uint16_t temperatureIndex;//this index is calculated from clock time and it is used to store next measurement
		uint8_t previousAssignStore;/*time in minute of last start storing measurement.
			Value used to protect against multiple storing day structure in the same minute. */
		uint8_t previousAssignFilter;/*time in minute of last gathered measurement.
			Value used to protect against multiple time gathered data in the same minute. */
		uint8_t valuesCounter;//number of temperature values in temperatureTableFilter
		uint16_t temperatureTableFilter[NUMBER_OF_MEASUREMENTS_IN_FILTER_TABLE];//table with temperature values gathered every 3 minutes
		uint8_t transactionId;/*unique transaction ID - all object with this type must contain different ID.
			It is used to recognize which temperature day structure(TemperatureSingleDayRecordType) is send to FRAM */
		uint8_t source;
	}TemperatureFramWriteTransactionPackageType;

	typedef struct
	{
		TemperatureSingleDayRecordType singleRecord;
		uint16_t framIndex; //index in FRAM where singleRecord occur
		bool availabilityFlag; //flag is set when data will be loaded
		bool notExistFlag; //flag is set when data will be not find in FRAM
		void *pointerToNextElement;
		void *pointerToPreviousElement;
	}ReadFramTempBufferType;

	typedef struct
	{
		TemperatureSingleDayRecordType temperatureSingleDayTmp;
		uint16_t searchFramIndexPosition;
		bool startReadTransaction;
		bool moveBackward;
		uint8_t searchCounter;
		uint8_t dayTmp;
		uint8_t monthTmp;
		uint8_t yearTmp;
		ReadFramTempBufferType *pointerToStructureTmp;
	}TemperatureFramReadTransactionPackageType;

	typedef struct
	{
		ReadFramTempBufferType *structPointer;
		uint8_t structIndex; //Index of temperature value inside single index
		uint8_t source; //the same value like in TemperatureSingleDayRecordType
		bool lockMoveFlag; //lock cursor when new data isn't available
		bool loadDataFlag; //flag for code inside thread which will start load data from FRAM
	}BufferCursorType;

	extern ClockStateType ClockState;
	extern WidgetsStringsType WidgetsStrings;
	extern TemperatureSingleDayRecordType TemperatureSingleDay[NUM_OF_TEMPERATURE_SOURCE];
	extern TemperatureFramWriteTransactionPackageType TemperatureFramTransactionSensorTable[NUM_OF_TEMPERATURE_SOURCE];
	extern UG_GUI gui;
	extern UG_WINDOW clockSettingsWindow;
	extern UG_WINDOW temperatureWindow;
	extern UG_WINDOW wifiSettingsWindow;
	extern ReadFramTempBufferType ReadFramTempBufferTable[READ_TEMP_FRAM_BUFFER_SIZE];
	extern BufferCursorType BufferCursor;
	extern TemperatureFramReadTransactionPackageType TemperatureFramReadTransaction;

	void GUI_ClockInit(void);
	void GUI_UpdateTemperature(void);
	void GUI_UpdateTime(void);
	void GUI_IncrementDay(uint8_t *day, uint8_t *month, uint8_t *year);
	void GUI_DecrementDay(uint8_t *day, uint8_t *month, uint8_t *year);
	uint16_t GUI_ReturnNewFramIndex(void);
	void GUI_InitTemperaureStructure(uint8_t source, uint8_t day, uint8_t month, uint8_t year, TemperatureSingleDayRecordType *pointerToStructure);
	void GUI_ProcessTemperatureWindow(void);
	void GUI_ProcessAlarmAnimation(void);
	uint16_t GUI_GetIncrementedFramIndex(uint16_t value);
	uint16_t GUI_GetDecrementedFramIndex(uint16_t value);
	uint16_t returnFramIndexValue(uint8_t numOfFlag);
	void GUI_IncrementSecond(void);
	void GUI_RefreshWifiWindow(void);
	uint8_t GUI_ReturnMaxDayInMonth(uint8_t month, uint8_t year);

#ifdef __cplusplus
}
#endif

#endif  /* _GUI_CLOCK_H_ */
