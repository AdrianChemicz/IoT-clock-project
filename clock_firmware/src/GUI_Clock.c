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

#include "GUI_Clock.h"
#include "Thread.h"
#include "LCD.h"

ClockStateType ClockState;
WidgetsStringsType WidgetsStrings;

TemperatureSingleDayRecordType TemperatureSingleDay[NUM_OF_TEMPERATURE_SOURCE];

TemperatureFramWriteTransactionPackageType TemperatureFramTransactionSensorTable[NUM_OF_TEMPERATURE_SOURCE];
TemperatureFramReadTransactionPackageType TemperatureFramReadTransaction;

ReadFramTempBufferType ReadFramTempBufferTable[READ_TEMP_FRAM_BUFFER_SIZE];
BufferCursorType BufferCursor;

static uint8_t* hourPointer = 0;
static uint8_t* minutePointer = 0;

UG_GUI gui; // Global GUI structure

static bool calendarModificationIsActive;
static CalendarValueInSettingsType CalendarValueInSettings;

static bool graphRightButtonWasPressed = false;
static bool graphLeftButtonWasPressed = false;

UG_WINDOW mainWindow;
UG_WINDOW clockSettingsWindow;
UG_WINDOW temperatureWindow;
UG_WINDOW settingsWindow;
UG_WINDOW wifiSettingsWindow;
UG_WINDOW wifiKeyboardWindow;

//main window
static UG_BUTTON buttonClockValue;

static UG_TEXTBOX textBoxTemperatureOutside;
static UG_TEXTBOX textBoxTemperatureInside;
static UG_TEXTBOX textBoxTemperatureFurnace;

static UG_BUTTON buttonSettings;

static UG_BUTTON buttonTemperatureOutside;
static UG_BUTTON buttonTemperatureInside;
static UG_BUTTON buttonTemperatureFurnace;
static UG_BUTTON buttonWiFiSettings;

static UG_IMAGE imageTemperatureOutside;
static UG_IMAGE imageTemperatureInside;
static UG_IMAGE imageTemperatureFurnace;
static UG_IMAGE imageWiFiSettings;

static UG_BUTTON buttonFirstAllarmStatus;
static UG_BUTTON buttonSecondAllarmStatus;
static UG_BUTTON buttonFurnaceAllarmStatus;

static UG_AREA possitionButtonFirstAlarm;
static UG_AREA possitionButtonSecondAlarm;
static UG_AREA possitionButtonFurnaceAlarm;

//clock settings window
static UG_TEXTBOX textBoxSettingInformation;
static UG_TEXTBOX textBoxTimeValue;
static UG_BUTTON buttonCloseClockSettingsWindow;

static UG_BUTTON buttonIncrementHour;
static UG_BUTTON buttonDecrementHour;
static UG_BUTTON buttonIncrementMinute;
static UG_BUTTON buttonDecrementMinute;

static UG_BUTTON buttonChoseTimeValue;
static UG_BUTTON buttonChoseFirstAlarm;
static UG_BUTTON buttonChoseSecondAlarm;

static UG_CHECKBOX ckeckBoxActiveFirstAlarm;
static UG_CHECKBOX ckeckBoxActiveSecondAlarm;

//temperature window
static UG_TEXTBOX textBoxTemperatureInformation;
static UG_TEXTBOX textBoxGraphLabel;
static UG_BUTTON buttonCloseTemperatureWindow;

static UG_BUTTON buttonHistoryGraphLeft;
static UG_BUTTON buttonHistoryGraphRight;

static UG_TEXTBOX textBoxStepValueLabel;
static UG_TEXTBOX textBoxTemperatureOffset;
static UG_CHECKBOX ckeckBoxTemperatureAlarm;
static UG_CHECKBOX ckeckBoxRecordTemperature;

static UG_BUTTON buttonIncrementTemperatureOffset;
static UG_BUTTON buttonDecrementTemperatureOffset;
static UG_BUTTON buttonIncrementTemperatureAlarm;
static UG_BUTTON buttonDecrementTemperatureAlarm;
static UG_BUTTON buttonIncrementGraphStep;
static UG_BUTTON buttonDecrementGraphStep;

static uint8_t temperatureGraphStepValue;

//options window
static UG_BUTTON buttonCloseOptionsWindow;
static UG_BUTTON buttonIncrementBrightness;
static UG_BUTTON buttonDecrementBrightness;
static UG_BUTTON buttonCalibrateLCD;
static UG_BUTTON buttonFactoryReset;
static UG_BUTTON buttonSetCalendar;
static UG_BUTTON buttonIncrementDay;
static UG_BUTTON buttonDecrementDay;
static UG_BUTTON buttonIncrementMonth;
static UG_BUTTON buttonDecrementMonth;
static UG_BUTTON buttonIncrementYear;
static UG_BUTTON buttonDecrementYear;

static UG_TEXTBOX textBoxBrightnessValueLabel;
static UG_TEXTBOX textBoxBrightnessValuePercents;
static UG_TEXTBOX textBoxUpTime;
static UG_TEXTBOX textBoxCalendar;

//wifi Settings window
static UG_TEXTBOX textBoxWifiStatus;
static UG_TEXTBOX textBoxApnListLabel;
static UG_TEXTBOX textBoxAssignedIpAddress;

static UG_TEXTBOX textBoxNetwork[NUMBER_OF_WIFI_DISPLAY_NETWORKS];
static UG_BUTTON buttonNetwork[NUMBER_OF_WIFI_DISPLAY_NETWORKS];

static UG_BUTTON buttonCloseWifiSettingsWindow;
static UG_BUTTON buttonControlConnection;
static UG_BUTTON buttonUpNetworkList;
static UG_BUTTON buttonDownNetworkList;

uint8_t wifiListCursorPosition;
static bool refreshWifiList;

//wifi Keyboard window
static UG_TEXTBOX textBoxApnLabel;
static UG_TEXTBOX textBoxChosenApnLabel;
static UG_TEXTBOX textBoxPApnPasswordLabel;
static UG_TEXTBOX textBoxPApnPasswordContent;
static UG_CHECKBOX ckeckBoxShowApnPassword;

static UG_BUTTON buttonCloseKeyboardWindow;
static UG_BUTTON buttonKeyQor1Num1a;
static UG_BUTTON buttonKeyWor2Num2a;
static UG_BUTTON buttonKeyEor3Num3a;
static UG_BUTTON buttonKeyRor4Num4a;
static UG_BUTTON buttonKeyTor5Num5a;
static UG_BUTTON buttonKeyYor6Num6a;
static UG_BUTTON buttonKeyUor7Num7a;
static UG_BUTTON buttonKeyIor8Num8a;
static UG_BUTTON buttonKeyOor9Num9a;
static UG_BUTTON buttonKeyPor0Num10a;
static UG_BUTTON buttonKeyGraveAccentOrTildeNum11a; // ` ~

static UG_BUTTON buttonKeyAorMonkeyNum1b; // A @
static UG_BUTTON buttonKeySorHashNum2b; // S #
static UG_BUTTON buttonKeyDorDolarNum3b; // D $
static UG_BUTTON buttonKeyForFloorNum4b; // F _
static UG_BUTTON buttonKeyGorAmpersandNum5b; // G &
static UG_BUTTON buttonKeyHorMinusNum6b; // H -
static UG_BUTTON buttonKeyJorPlusNum7b; // J +
static UG_BUTTON buttonKeyKorAsteriskNum8b; // K *
static UG_BUTTON buttonKeyLorExclamationMarkNum9b; // L !
static UG_BUTTON buttonKeyRightBracesOrRightSquareBracketsNum10b; // } ]

static UG_BUTTON buttonKeyShiftNum1c; // Shift
static UG_BUTTON buttonKeyZorQuotationMarksSingleNum2c; // Z '
static UG_BUTTON buttonKeyXorQuotationMarksDoubleNum3c; // X "
static UG_BUTTON buttonKeyCorQuestionMarkNum4c; // C ?
static UG_BUTTON buttonKeyVorEqualsSignNum5c; // V =
static UG_BUTTON buttonKeyBorBackslashNum6c; /* B \ */
static UG_BUTTON buttonKeyNorPercent1Num7c; // N %
static UG_BUTTON buttonKeyMorForwardSlashNum8c; // M /
static UG_BUTTON buttonKeyLeftBracesOrLeftSquareBracketsNum9c; // { [
static UG_BUTTON buttonKeyCaretOrVerticalBarNum10c; // ^ |
static UG_BUTTON buttonKeyBackspaceNum11c; //Backspace

static UG_BUTTON buttonKeySwitchToSignNum1d; // Shift
static UG_BUTTON buttonKeyDotOrColonNum2d; // . :
static UG_BUTTON buttonKeyCommaQorSemicolonNum3d; // , ;
static UG_BUTTON buttonKeySpaceNum4d; // Space
static UG_BUTTON buttonKeyLeftRoundBracketOrLeftChevronNum5d; // ( <
static UG_BUTTON buttonKeyRightRoundBracketOrRightChevronNum6d; // ) >
static UG_BUTTON buttonKeyEnterNum7d; // Enter

static UG_OBJECT mainWindowObjects[MAX_OBJECTS];
static UG_OBJECT clockSettingsObjects[MAX_OBJECTS];
static UG_OBJECT temperatureWindowObjects[MAX_OBJECTS];
static UG_OBJECT settingsWindowObjects[MAX_OBJECTS_SETTINGS];
static UG_OBJECT wifiSettingsObjects[MAX_OBJECTS_WIFI_SETTINGS];
static UG_OBJECT wifiKeyboardObjects[MAX_OBJECTS_KEYBOARD_WINDOW];

KEYBOARD_SIGNS KeyboardSignsGlobalState;

uint8_t sellectedWifiNetworkName[33];
uint8_t passwordToApnBuffer[MAX_APN_PASSWORD_LENGTH];

const uint8_t KeyboardCodeTable[NUMBER_OF_KEY_ON_KEYBOARD][6] = { { 'q', 0, 'Q', 0, '1', 0 }, { 'w', 0, 'W', 0, '2', 0 }, { 'e', 0, 'E', 0, '3', 0 }, { 'r', 0, 'R', 0, '4', 0 },
{ 't', 0, 'T', 0, '5', 0 }, { 'y', 0, 'Y', 0, '6', 0 }, { 'u', 0, 'U', 0, '7', 0 }, { 'i', 0, 'I', 0, '8', 0 }, { 'o', 0, 'O', 0, '9', 0 }, { 'p', 0, 'P', 0, '0', 0 },
{ '`', 0, '`', 0, '~', 0 }, { 'a', 0, 'A', 0, '@', 0 }, { 's', 0, 'S', 0, '#', 0 }, { 'd', 0, 'D', 0, '$', 0 }, { 'f', 0, 'F', 0, '_', 0 }, { 'g', 0, 'G', 0, '&', 0 },
{ 'h', 0, 'H', 0, '-', 0 }, { 'j', 0, 'J', 0, '+', 0 }, { 'k', 0, 'K', 0, '*', 0 }, { 'l', 0, 'L', 0, '!', 0 }, { '}', 0, '}', 0, ']', 0 },
{ 0, 0, 0, 0, 0, 0 }, { 'z', 0, 'Z', 0, '\'', 0 }, { 'x', 0, 'X', 0, '"', 0 }, { 'c', 0, 'C', 0, '?', 0 }, { 'v', 0, 'V', 0, '=', 0 }, { 'b', 0, 'B', 0, '\\', 0 },
{ 'n', 0, 'N', 0, '%', 0 }, { 'm', 0, 'M', 0, '/', 0 }, { '{', 0, '{', 0, '[', 0 }, { '^', 0, '^', 0, '|', 0 }, { 0, 0, 0, 0, 0, 0 },
{ 0, 0, 0, 0, 0, 0 }, { '.', 0, '.', 0, ':', 0 }, { ',', 0, ',', 0, ';', 0 }, { ' ', 0, ' ', 0, ' ', 0 }, { '(', 0, '(', 0, '<', 0 }, { ')', 0, ')', 0, '>', 0 },
{ 0, 0, 0, 0, 0, 0 } };

static void changeHourValue(uint8_t* variableAdress, TYPE_OF_OPERATION typeOfOperation)
{
	if (typeOfOperation == INCREMENT)
	{
		(*variableAdress)++;
		if (*variableAdress == 24)
			*variableAdress = 0;
	}
	if (typeOfOperation == DECREMENT)
	{
		(*variableAdress)--;
		if (*variableAdress == 255)
			*variableAdress = 23;
	}
}

static void changeMinuteValue(uint8_t* variableAdress, uint8_t typeOfOperation)
{
	if (typeOfOperation == INCREMENT)
	{
		(*variableAdress)++;
		if (*variableAdress == 60)
			*variableAdress = 0;
	}
	if (typeOfOperation == DECREMENT)
	{
		(*variableAdress)--;
		if (*variableAdress == 255)
			*variableAdress = 59;
	}
}

static void calculateTimeString(uint8_t* destinationString, uint8_t hour, uint8_t minute)
{
	uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0' };

	//asembly time using data in clock structure
	strcpy(destinationString, "\0");

	if (hour < 10)
		strcat(destinationString, "0");

	itoa(hour, numberTmp, 10);
	strcat(destinationString, numberTmp);
	strcat(destinationString, " : ");

	if (minute < 10)
		strcat(destinationString, "0");

	itoa(minute, numberTmp, 10);
	strcat(destinationString, numberTmp);
}

static void calculateTemperatureString(uint8_t* labelTemperature, uint16_t temperatureValue, bool addDegree)
{
	uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0' };
	int16_t signedTemperatureTmp = (int16_t)temperatureValue - (int16_t)(TEMPERATURE_OFFSET_FROM_ZERO);

	if(temperatureValue == INVALID_READ_SENSOR_VALUE)
	{
		strcpy(labelTemperature, "--,-\0");
	}
	else
	{
		itoa((signedTemperatureTmp/10), numberTmp, 10);
		strcpy(labelTemperature, numberTmp);
		strcat(labelTemperature, ",");
		itoa((abs(signedTemperatureTmp)%10), numberTmp, 10);
		strcat(labelTemperature, numberTmp);
	}

	if(addDegree)
	{
		strcat(labelTemperature, "\xF8\C\0");
	}
}

static void calculateTitleStringInTemperatureWindow(void)
{
	uint8_t numberTmp[10] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

	switch (ClockState.temperatureTypeInWindow)
	{
	case OUTSIDE_TEMPERATURE:
		strcpy(WidgetsStrings.labelTemperatureInTitleOfWindow, "temperature outside: ");
		break;

	case INSIDE_TEMPERATURE:
		strcpy(WidgetsStrings.labelTemperatureInTitleOfWindow, "temperature in building: ");
		break;

	case FURNACE_TEMPERATURE:
		strcpy(WidgetsStrings.labelTemperatureInTitleOfWindow, "temperature on furnace: ");
		break;

	default:
		strcpy(WidgetsStrings.labelTemperatureInTitleOfWindow, "\0");
		break;
	}

	calculateTemperatureString(numberTmp, ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].temperatureValue,
		true);

	strcat(WidgetsStrings.labelTemperatureInTitleOfWindow, numberTmp);
}

static void setRecordCheckboxInTemperatureWindow(bool state)
{
	if(state)
		UG_CheckboxSetCheched(&temperatureWindow, CHB_ID_1, CHB_STATE_PRESSED);
	else
		UG_CheckboxSetCheched(&temperatureWindow, CHB_ID_1, CHB_STATE_RELEASED);
}

static void calculateTemperatureOffsetString(void)
{
	uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0'};

	strcpy(WidgetsStrings.labelTemperatureFurnaceOffset, "temperature offset: ");
	itoa(ClockState.temperatureFurnaceOffset, numberTmp, 10);
	strcat(WidgetsStrings.labelTemperatureFurnaceOffset, numberTmp);
	strcat(WidgetsStrings.labelTemperatureFurnaceOffset, "\xF8\C");
}

static void calculateTemperatureAlarmString(void)
{
	uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0' };

	strcpy(WidgetsStrings.labelTemperatureFurnaceAlarm, "alarm at: ");
	itoa(ClockState.temperatureFurnaceAlarmThreshold, numberTmp, 10);
	strcat(WidgetsStrings.labelTemperatureFurnaceAlarm, numberTmp);
	strcat(WidgetsStrings.labelTemperatureFurnaceAlarm, "\xF8\C");
}

static void visibleTemperatureSettings(bool visible)
{
	if (visible == true)
	{
		UG_TextboxShow(&temperatureWindow, TXB_ID_2);
		UG_CheckboxShow(&temperatureWindow, CHB_ID_0);
		UG_ButtonShow(&temperatureWindow, BTN_ID_2);
		UG_ButtonShow(&temperatureWindow, BTN_ID_3);
		UG_ButtonShow(&temperatureWindow, BTN_ID_4);
		UG_ButtonShow(&temperatureWindow, BTN_ID_5);
	}
	else
	{
		UG_TextboxHide(&temperatureWindow, TXB_ID_2);
		UG_CheckboxHide(&temperatureWindow, CHB_ID_0);
		UG_ButtonHide(&temperatureWindow, BTN_ID_2);
		UG_ButtonHide(&temperatureWindow, BTN_ID_3);
		UG_ButtonHide(&temperatureWindow, BTN_ID_4);
		UG_ButtonHide(&temperatureWindow, BTN_ID_5);
	}
}

static void calculateGraphStepValueString(uint8_t value)
{
	uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0' };
	strcpy(WidgetsStrings.labelGraphStep, "steps: ");

	itoa(value, numberTmp, 10);
	strcat(WidgetsStrings.labelGraphStep, numberTmp);
}

static void calculateUpTimeString(void)
{
	uint8_t numberTmp[10] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

	if(ClockState.systemUpTime == 0)
	{
		strcpy(WidgetsStrings.labelUpTime, "uptime: 0d 0h 0min");
	}
	else
	{
		strcpy(WidgetsStrings.labelUpTime, "uptime: ");
		itoa((ClockState.systemUpTime/86400), numberTmp, 10);//number of day
		strcat(WidgetsStrings.labelUpTime, numberTmp);
		strcat(WidgetsStrings.labelUpTime, "d ");
		itoa(((ClockState.systemUpTime%86400)/3600), numberTmp, 10);//number of h
		strcat(WidgetsStrings.labelUpTime, numberTmp);
		strcat(WidgetsStrings.labelUpTime, "h ");
		itoa(((ClockState.systemUpTime%3600)/60), numberTmp, 10);//number of minutes
		strcat(WidgetsStrings.labelUpTime, numberTmp);
		strcat(WidgetsStrings.labelUpTime, "min");
	}
}

static void setCalendarModification(bool status)
{
	if (status)
	{
		UG_ButtonShow(&settingsWindow, BTN_ID_6);
		UG_ButtonShow(&settingsWindow, BTN_ID_7);
		UG_ButtonShow(&settingsWindow, BTN_ID_8);
		UG_ButtonShow(&settingsWindow, BTN_ID_9);
		UG_ButtonShow(&settingsWindow, BTN_ID_10);
		UG_ButtonShow(&settingsWindow, BTN_ID_11);
		UG_ButtonSetText(&settingsWindow, BTN_ID_5, "save");
	}
	else
	{
		UG_ButtonHide(&settingsWindow, BTN_ID_6);
		UG_ButtonHide(&settingsWindow, BTN_ID_7);
		UG_ButtonHide(&settingsWindow, BTN_ID_8);
		UG_ButtonHide(&settingsWindow, BTN_ID_9);
		UG_ButtonHide(&settingsWindow, BTN_ID_10);
		UG_ButtonHide(&settingsWindow, BTN_ID_11);
		UG_ButtonSetText(&settingsWindow, BTN_ID_5, "change");
	}
}

uint8_t GUI_ReturnMaxDayInMonth(uint8_t month, uint8_t year)
{
	switch (month)
	{
	case 2:
		if(year % 4 == 0 && year % 100 != 0 || year % 400 == 0)
			return 29;
		else
			return 28;

	case 4:
	case 6:
	case 9:
	case 11:
		return 30;

	default:
		return 31;
	}
}

static void calculateCalendarString(uint8_t* label, uint8_t day, uint8_t month, uint8_t year, uint8_t* separationSign)
{
	uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0' };

	//days
	itoa(day, numberTmp, 10);

	if (day == INVALID_CALENDAR_DATE)
	{
		strcpy(label, "__");
	}
	else if (day < 10)
	{
		strcpy(label, "0");
		strcat(label, numberTmp);
	}
	else
	{
		strcpy(label, numberTmp);
	}

	strcat(label, separationSign);

	//months
	itoa(month, numberTmp, 10);

	if (month == INVALID_CALENDAR_DATE)
	{
		strcat(label, "__");
	}
	else if (month < 10)
	{
		strcat(label, "0");
		strcat(label, numberTmp);
	}
	else
	{
		strcat(label, numberTmp);
	}

	strcat(label, separationSign);

	//years
	itoa(year, numberTmp, 10);

	if (year == INVALID_CALENDAR_DATE)
	{
		strcat(label, "____");
	}
	else if ((year < 100) && (year >= 10))
	{
		strcat(label, "20");
		strcat(label, numberTmp);
	}
	else if (year < 10)
	{
		strcat(label, "200");
		strcat(label, numberTmp);
	}
	else
	{
		strcat(label, numberTmp);
	}
}

static void initValueOperation(uint8_t *valuePointer)
{
	if (*valuePointer == 255)
	{
		*valuePointer = 0;
	}
}

static void validMaxDayCalendar(void)
{
	if (CalendarValueInSettings.day > GUI_ReturnMaxDayInMonth(CalendarValueInSettings.month,
		CalendarValueInSettings.year))
	{
		CalendarValueInSettings.day = GUI_ReturnMaxDayInMonth(CalendarValueInSettings.month,
			CalendarValueInSettings.year);
	}
}

static void setKeyboardButtons(KEYBOARD_SIGNS KeyboardSigns)
{
	for (int i = 0; i < NUMBER_OF_KEY_ON_KEYBOARD; i++)
	{
		if (strlen(KeyboardCodeTable[i]) != 0)
		{
			switch (KeyboardSigns)
			{
			case SMALL_LETTER:
				UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + i), KeyboardCodeTable[i]);
				break;

			case BIG_LETTER:
				UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + i), &KeyboardCodeTable[i][2]);
				break;

			case SPECIAL_CHARACTER:
				UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + i), &KeyboardCodeTable[i][4]);
				break;

			default:
				break;
			}
			UG_ButtonSetFont(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + i), &FONT_8X12);
		}

		UG_ButtonSetStyle(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + i), BTN_STYLE_2D);
	}/* for (int i = 0; i < NUMBER_OF_KEY_ON_KEYBOARD; i++) */
}

static void generatePasswordString(uint8_t *displayedString, uint8_t *passwordString, uint8_t checkBoxState)
{
	//clear displayedString
	memset(displayedString, '\0', MAX_APN_PASSWORD_LENGTH);

	if (strlen(passwordString) != 0)
	{
		if (checkBoxState == CHB_STATE_PRESSED)
		{
			strcpy(displayedString, passwordString);
		}
		else
		{
			if (strlen(passwordString) > 1)
			{
				memset(displayedString, '*', (strlen(passwordString) - 1));
			}

			strcat(displayedString, &passwordString[strlen(passwordString) - 1]);
		}
	}
}

void mainWindowHandler(UG_MESSAGE *msg)
{
	if (msg->type == MSG_TYPE_OBJECT)
	{
		if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED)
		{
			switch (msg->sub_id)
			{
			/* clock Button was pressed */
			case BTN_ID_0:
				//init window using data from structure
				ClockState.activeTimeSettings = CURRENT_TIME;
				hourPointer = &ClockState.currentTimeHour;
				minutePointer = &ClockState.currentTimeMinute;
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_0, "actual time:");
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, ClockState.currentTimeHour, ClockState.currentTimeMinute);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				UG_WindowShow(&clockSettingsWindow);
				break;
			/* settings button was released */
			case BTN_ID_1:
				CalendarValueInSettings.day = ClockState.day;
				CalendarValueInSettings.month = ClockState.month;
				CalendarValueInSettings.year = ClockState.year;

				calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
					CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
				UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);

				UG_WindowShow(&settingsWindow);
				break;
			/* temperatureOutside button was pressed */
			case BTN_ID_2:
				visibleTemperatureSettings(false);
				ClockState.temperatureTypeInWindow = OUTSIDE_TEMPERATURE;
				calculateTitleStringInTemperatureWindow();
				setRecordCheckboxInTemperatureWindow(ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].recordTemperature);
				UG_TextboxSetText(&temperatureWindow, TXB_ID_0, WidgetsStrings.labelTemperatureInTitleOfWindow);
				UG_WindowShow(&temperatureWindow);
				break;
			/* temperatureInside button was pressed */
			case BTN_ID_3:
				visibleTemperatureSettings(false);
				ClockState.temperatureTypeInWindow = INSIDE_TEMPERATURE;
				calculateTitleStringInTemperatureWindow();
				setRecordCheckboxInTemperatureWindow(ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].recordTemperature);
				UG_TextboxSetText(&temperatureWindow, TXB_ID_0, WidgetsStrings.labelTemperatureInTitleOfWindow);
				UG_WindowShow(&temperatureWindow);
				break;
			/* temperatureFurnace button was pressed */
			case BTN_ID_4:
				visibleTemperatureSettings(true);
				ClockState.temperatureTypeInWindow = FURNACE_TEMPERATURE;
				calculateTitleStringInTemperatureWindow();
				setRecordCheckboxInTemperatureWindow(ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].recordTemperature);
				UG_TextboxSetText(&temperatureWindow, TXB_ID_0, WidgetsStrings.labelTemperatureInTitleOfWindow);
				UG_WindowShow(&temperatureWindow);
				break;
			/* WiFi button was pressed */
			case BTN_ID_5:
				UG_WindowShow(&wifiSettingsWindow);
				break;
			/* first alarm status button was pressed */
			case BTN_ID_6:
				ClockState.firstAlarmRaised = false;
				break;
			/* second alarm status button was pressed */
			case BTN_ID_7:
				ClockState.secondAlarmRaised = false;
				break;
			/* furnace alarm status button was pressed */
			case BTN_ID_8:
				ClockState.temperatureFurnaceAlarmRaised = false;
				break;
			}
		}
	}
}

void clockSettingsWindowHandler(UG_MESSAGE *msg)
{
	if (msg->type == MSG_TYPE_OBJECT)
	{
		if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED)
		{
			switch (msg->sub_id)
			{
			/* Increment hour button was pressed */
			case BTN_ID_0:
				changeHourValue(hourPointer, INCREMENT);
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				break;
			/* Decrement hour button was pressed */
			case BTN_ID_1:
				changeHourValue(hourPointer, DECREMENT);
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				break;
			/* Increment minute button was pressed */
			case BTN_ID_2:
				changeMinuteValue(minutePointer, INCREMENT);
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				break;
			/* Decrement minute button was pressed */
			case BTN_ID_3:
				changeMinuteValue(minutePointer, DECREMENT);
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				break;
			/* Current time button was pressed */
			case BTN_ID_4:
				ClockState.activeTimeSettings = CURRENT_TIME;
				hourPointer = &ClockState.currentTimeHour;
				minutePointer = &ClockState.currentTimeMinute;
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_0, "actual time:");
				break;
			/* First alarm button was pressed */
			case BTN_ID_5:
				ClockState.activeTimeSettings = FIRST_ALARM;
				hourPointer = &ClockState.firstAlarmHour;
				minutePointer = &ClockState.firstAlarmMinute;
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_0, "first alarm:");
				break;
			/* Second alarm button was pressed */
			case BTN_ID_6:
				ClockState.activeTimeSettings = SECOND_ALARM;
				hourPointer = &ClockState.secondAlarmHour;
				minutePointer = &ClockState.secondAlarmMinute;
				calculateTimeString(WidgetsStrings.labelClockSettingsTimeValue, *hourPointer, *minutePointer);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, WidgetsStrings.labelClockSettingsTimeValue);
				UG_TextboxSetText(&clockSettingsWindow, TXB_ID_0, "second alarm:");
				break;
			/* Close button was pressed */
			case BTN_ID_7:
				//assign state of allarm checkbox to appropriate variable
				if (UG_CheckboxGetChecked(&clockSettingsWindow, CHB_ID_0) == CHB_STATE_PRESSED)
					ClockState.firstAlarmActive = true;
				else
					ClockState.firstAlarmActive = false;

				if (UG_CheckboxGetChecked(&clockSettingsWindow, CHB_ID_1) == CHB_STATE_PRESSED)
					ClockState.secondAlarmActive = true;
				else
					ClockState.secondAlarmActive = false;

				calculateTimeString(WidgetsStrings.labelMainWindowClockTimeValue, ClockState.currentTimeHour, ClockState.currentTimeMinute);
				UG_ButtonSetText(&mainWindow, BTN_ID_0, WidgetsStrings.labelMainWindowClockTimeValue);

				UG_WindowShow(&mainWindow);
				break;
			}
		}
	}
}

void temperatureWindowHandler(UG_MESSAGE *msg)
{
	if (msg->type == MSG_TYPE_OBJECT)
	{
		if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED)
		{
			switch (msg->sub_id)
			{
			/* Close button was pressed */
			case BTN_ID_6:
				//check record checkbox if calendar day was set
				if(ClockState.day != INVALID_CALENDAR_DATE && ClockState.month != INVALID_CALENDAR_DATE
					&& ClockState.year != INVALID_CALENDAR_DATE)
				{
					//assign register temperature checkbox state to appropriate ClockState field
					bool recordStateValue = false;
					if(UG_CheckboxGetChecked(&temperatureWindow, CHB_ID_1) == CHB_STATE_PRESSED)
						recordStateValue = true;
					else
						recordStateValue = false;

					if(ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].recordTemperature == false
						&& recordStateValue == true)
					{
						ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].recordTemperature = true;
						ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].temperatureFramIndex = GUI_ReturnNewFramIndex();
						GUI_InitTemperatureStructure(ClockState.temperatureTypeInWindow, ClockState.day,
							ClockState.month, ClockState.year, &TemperatureSingleDay[ClockState.temperatureTypeInWindow]);
					}
				}

				//set Furnace alarm
				if(ClockState.temperatureTypeInWindow == FURNACE_TEMPERATURE)
				{
					if (UG_CheckboxGetChecked(&temperatureWindow, CHB_ID_0) == CHB_STATE_PRESSED)
						ClockState.temperatureFurnaceAlarmActive = true;
					else
						ClockState.temperatureFurnaceAlarmActive = false;
				}

				UG_WindowShow(&mainWindow);
				break;
			/* history graph left button was pressed */
			case BTN_ID_0:
				graphLeftButtonWasPressed = true;
				break;
			/* history graph right button was pressed */
			case BTN_ID_1:
				graphRightButtonWasPressed = true;
				break;
			/* Increment temperature offset button was pressed */
			case BTN_ID_2:
				ClockState.temperatureFurnaceOffset++;
				calculateTemperatureOffsetString();
				UG_TextboxSetText(&temperatureWindow, TXB_ID_2, WidgetsStrings.labelTemperatureFurnaceOffset);
				break;
			/* Decrement temperature offset button was pressed */
			case BTN_ID_3:
				ClockState.temperatureFurnaceOffset--;
				calculateTemperatureOffsetString();
				UG_TextboxSetText(&temperatureWindow, TXB_ID_2, WidgetsStrings.labelTemperatureFurnaceOffset);
				break;
			/* Increment temperature alarm button was pressed */
			case BTN_ID_4:
				ClockState.temperatureFurnaceAlarmThreshold++;
				if (ClockState.temperatureFurnaceAlarmThreshold > 99)
					ClockState.temperatureFurnaceAlarmThreshold = 99;
				calculateTemperatureAlarmString();
				UG_CheckboxSetText(&temperatureWindow, CHB_ID_0, WidgetsStrings.labelTemperatureFurnaceAlarm);
				break;
			/* Decrement temperature alarm button was pressed */
			case BTN_ID_5:
				ClockState.temperatureFurnaceAlarmThreshold--;
				if (ClockState.temperatureFurnaceAlarmThreshold == INVALID_READ_SENSOR_VALUE)
					ClockState.temperatureFurnaceAlarmThreshold = 0;
				calculateTemperatureAlarmString();
				UG_CheckboxSetText(&temperatureWindow, CHB_ID_0, WidgetsStrings.labelTemperatureFurnaceAlarm);
				break;
			/* Decrement step button was pressed */
			case BTN_ID_7:
				if (temperatureGraphStepValue > 1)
				{
					temperatureGraphStepValue--;
					calculateGraphStepValueString(temperatureGraphStepValue);
					UG_TextboxSetText(&temperatureWindow, TXB_ID_3, WidgetsStrings.labelGraphStep);
				}
				break;
			/* Increment step button was pressed */
			case BTN_ID_8:
				if (temperatureGraphStepValue < MAX_STEP_ON_GRAPH)
				{
					temperatureGraphStepValue++;
					calculateGraphStepValueString(temperatureGraphStepValue);
					UG_TextboxSetText(&temperatureWindow, TXB_ID_3, WidgetsStrings.labelGraphStep);
				}
				break;
			}
		}
	}
}

void settingsWindowHandler(UG_MESSAGE *msg)
{
	if (msg->type == MSG_TYPE_OBJECT)
	{
		if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED)
		{
			switch (msg->sub_id)
			{
			/* Close button was release */
			case BTN_ID_0:
				setCalendarModification(false);
				calendarModificationIsActive = false;
				UG_WindowShow(&mainWindow);
				break;
			/* Decrement button was release */
			case BTN_ID_1:
				//decrement one
				ClockState.brightness--;
				if(ClockState.brightness == 0)
					ClockState.brightness = 1;

				Backlight_SetBrightness(ClockState.brightness);

				itoa(ClockState.brightness, WidgetsStrings.labelBrightnessValue, 10);
				UG_TextboxSetText(&settingsWindow, TXB_ID_1, WidgetsStrings.labelBrightnessValue);
				break;
			/* Increment button was release */
			case BTN_ID_2:
				//add five
				ClockState.brightness+=5;
				if(ClockState.brightness > 100)
					ClockState.brightness = 100;

				Backlight_SetBrightness(ClockState.brightness);

				itoa(ClockState.brightness, WidgetsStrings.labelBrightnessValue, 10);
				UG_TextboxSetText(&settingsWindow, TXB_ID_1, WidgetsStrings.labelBrightnessValue);
				break;
			/* Calibrate touch screen button was release */
			case BTN_ID_3:
				ClockInitTouchScreen();

				TouchPanel_SetCalibrationParameter(ClockState.x1RawCalibrationValue, ClockState.x2RawCalibrationValue,
					ClockState.y1RawCalibrationValue, ClockState.y2RawCalibrationValue);

				UG_WindowShow(&settingsWindow);
				break;
			/* factory Reset button was release */
			case BTN_ID_4:
				ClockState.factoryResetViaGui = true;
				break;
			/* buttonSetCalendar was release */
			case BTN_ID_5:
				/* change calendar settings was release */
				if (!calendarModificationIsActive)
				{
					setCalendarModification(true);
					calendarModificationIsActive = true;
				}
				else/* save calendar settings was release */
				{
					setCalendarModification(false);
					calendarModificationIsActive = false;

					//copy data from CalendarValueInSettings to ClockState
					ClockState.day = CalendarValueInSettings.day;
					ClockState.month = CalendarValueInSettings.month;
					ClockState.year = CalendarValueInSettings.year;
				}

				break;
			/* increment day was release */
			case BTN_ID_6:
				initValueOperation(&CalendarValueInSettings.day);
				CalendarValueInSettings.day++;
				validMaxDayCalendar();

				calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
					CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
				UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);

				break;
			/* decrement day was release */
			case BTN_ID_7:
				initValueOperation(&CalendarValueInSettings.day);
				if (CalendarValueInSettings.day > 1)
				{
					CalendarValueInSettings.day--;

					calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
						CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
					UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);
				}
				break;
			/* increment month was release */
			case BTN_ID_8:
				initValueOperation(&CalendarValueInSettings.month);
				if (CalendarValueInSettings.month < 12)
				{
					CalendarValueInSettings.month++;

					validMaxDayCalendar();

					calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
						CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
					UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);
				}
				break;
			/* decrement month was release */
			case BTN_ID_9:
				initValueOperation(&CalendarValueInSettings.month);
				if (CalendarValueInSettings.month > 1)
					CalendarValueInSettings.month--;

				validMaxDayCalendar();
				calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
					CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
				UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);

				break;
			/* increment year was release */
			case BTN_ID_10:
				initValueOperation(&CalendarValueInSettings.year);
				if (CalendarValueInSettings.year < 243)
				{
					CalendarValueInSettings.year++;

					validMaxDayCalendar();

					calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
						CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
					UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);
				}

				break;
			/* decrement year was release */
			case BTN_ID_11:
				initValueOperation(&CalendarValueInSettings.year);
				if (CalendarValueInSettings.year > 1)
					CalendarValueInSettings.year--;

				validMaxDayCalendar();
				calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
					CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
				UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);

				break;
			}
		}
	}
}

void wifiSettingsWindowHandler(UG_MESSAGE *msg)
{
	if (msg->type == MSG_TYPE_OBJECT)
	{
		if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED)
		{
			switch (msg->sub_id)
			{
				/* Close button was release */
			case BTN_ID_0:
				UG_WindowShow(&mainWindow);
				break;
				/* disconnect button was release */
			case BTN_ID_1:
				ClockState.wifiStartDisconnect = true;

				//clear password and ssid from clockstate
				memset(ClockState.ssidOfAssignedApn, '\0', SSID_STRING_LENGTH);
				memset(ClockState.passwordToAssignedApn, '\0', MAX_APN_PASSWORD_LENGTH);

				ClockState.wifiConnected = false;

				UG_WindowShow(&mainWindow);
				break;
				/* up network list was release */
			case BTN_ID_8:
				wifiListCursorPosition--;
				refreshWifiList = true;
				break;
				/* down network list was release */
			case BTN_ID_9:
				wifiListCursorPosition++;
				refreshWifiList = true;
				break;
			}/* switch (msg->sub_id) */

			/* Connect to Wifi was release */
			for (int i = 0; i < NUMBER_OF_WIFI_DISPLAY_NETWORKS; i++)
			{
				if (msg->sub_id == (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i))
				{
					if(ClockState.wifiConnected &&
						(strcmp(ApnStructure.AppInformationTable[wifiListCursorPosition + i].ssid, ClockState.ssidOfAssignedApn) == 0))
					{
						//Do nothing if device is already connected to this network
					}
					else
					{
						//copy selected APN to temporary buffer and init widget in new window
						memcpy(sellectedWifiNetworkName, ApnStructure.AppInformationTable[wifiListCursorPosition + i].ssid, 33);
						UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_1, sellectedWifiNetworkName);
						UG_WindowShow(&wifiKeyboardWindow);
					}
				}
			}/* for (int i = 0; i < NUMBER_OF_WIFI_DISPLAY_NETWORKS; i++) */
		}/* if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED) */
	}/* if (msg->type == MSG_TYPE_OBJECT) */
}

void wifiKeyboardWindowHandler(UG_MESSAGE *msg)
{
	if (msg->type == MSG_TYPE_OBJECT)
	{
		if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED)
		{
			switch (msg->sub_id)
			{
				/* Close button was release */
			case BTN_ID_0:
				UG_WindowShow(&wifiSettingsWindow);
				break;
				/* Shift button was release */
			case (BUTTON_WIFI_KEYBOARD_BEGIN + SHIFT_KEY_OFFSET_WKW):

				if (KeyboardSignsGlobalState == BIG_LETTER)
				{
					KeyboardSignsGlobalState = SMALL_LETTER;
				}
				else if (KeyboardSignsGlobalState == SMALL_LETTER)
				{
					KeyboardSignsGlobalState = BIG_LETTER;
				}

				setKeyboardButtons(KeyboardSignsGlobalState);

				break;
				/* Letter or special character button was release */
			case (BUTTON_WIFI_KEYBOARD_BEGIN + LETTER_OR_SPECIAL_CHAR_KEY_OFFSET_WKW):

				if (KeyboardSignsGlobalState == BIG_LETTER || KeyboardSignsGlobalState == SMALL_LETTER)
				{
					KeyboardSignsGlobalState = SPECIAL_CHARACTER;
				}
				else if (KeyboardSignsGlobalState == SPECIAL_CHARACTER)
				{
					KeyboardSignsGlobalState = SMALL_LETTER;
				}

				setKeyboardButtons(KeyboardSignsGlobalState);

				break;
				/* Enter button was release */
			case (BUTTON_WIFI_KEYBOARD_BEGIN + ENTER_KEY_OFFSET_WKW):
				//copy password and selected APN to ClockState.
				strcpy(ClockState.ssidOfAssignedApn, sellectedWifiNetworkName);
				strcpy(ClockState.passwordToAssignedApn, passwordToApnBuffer);
				UG_WindowShow(&mainWindow);
				break;
				/* Backspace button was release */
			case (BUTTON_WIFI_KEYBOARD_BEGIN + BACKSPACE_KEY_OFFSET_WKW):
				if (strlen(passwordToApnBuffer) > 0)
				{
					passwordToApnBuffer[strlen(passwordToApnBuffer) - 1] = '\0';
				}

				generatePasswordString(WidgetsStrings.labelApnPassword, passwordToApnBuffer,
					UG_CheckboxGetChecked(&wifiKeyboardWindow, CHB_ID_0));
				UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_3, WidgetsStrings.labelApnPassword);

				break;
			}/* switch (msg->sub_id) */

			for (int i = 0; i < NUMBER_OF_KEY_ON_KEYBOARD; i++)
			{
				if (msg->sub_id == (BUTTON_WIFI_KEYBOARD_BEGIN + i))
				{
					if (strlen(KeyboardCodeTable[i]) != 0)
					{
						switch (KeyboardSignsGlobalState)
						{
						case SMALL_LETTER:
							strcat(passwordToApnBuffer, KeyboardCodeTable[i]);
							break;

						case BIG_LETTER:
							strcat(passwordToApnBuffer, &KeyboardCodeTable[i][2]);
							break;

						case SPECIAL_CHARACTER:
							strcat(passwordToApnBuffer, &KeyboardCodeTable[i][4]);
							break;
						}

						generatePasswordString(WidgetsStrings.labelApnPassword, passwordToApnBuffer,
							UG_CheckboxGetChecked(&wifiKeyboardWindow, CHB_ID_0));
						UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_3, WidgetsStrings.labelApnPassword);
					}
				}
			}/* for (int i = 0; i < NUMBER_OF_KEY_ON_KEYBOARD; i++) */
		}/* if (msg->id == OBJ_TYPE_BUTTON && msg->event == OBJ_EVENT_RELEASED) */

		if (msg->id == OBJ_TYPE_CHECKBOX && msg->event == OBJ_EVENT_RELEASED)
		{
			if (msg->sub_id == CHB_ID_0)
			{
				generatePasswordString(WidgetsStrings.labelApnPassword, passwordToApnBuffer,
					UG_CheckboxGetChecked(&wifiKeyboardWindow, CHB_ID_0));
				UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_3, WidgetsStrings.labelApnPassword);
			}
		}
	}
}

void GUI_UpdateTemperature(void)
{
	//update temperature in main window
	calculateTemperatureString(WidgetsStrings.labelTemperatureInside,
		ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValue, true);
	UG_TextboxSetText(&mainWindow, TXB_ID_2, WidgetsStrings.labelTemperatureInside);

	calculateTemperatureString(WidgetsStrings.labelTemperatureOutside,
		ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValue, true);
	UG_TextboxSetText(&mainWindow, TXB_ID_1, WidgetsStrings.labelTemperatureOutside);

	calculateTemperatureString(WidgetsStrings.labelTemperatureFurnace,
		ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue, true);
	UG_TextboxSetText(&mainWindow, TXB_ID_3, WidgetsStrings.labelTemperatureFurnace);

	//update temperature in temperature window only if this window is visible
	if(gui.active_window == &temperatureWindow)
	{
		calculateTitleStringInTemperatureWindow();
		UG_TextboxSetText(&temperatureWindow, TXB_ID_0, WidgetsStrings.labelTemperatureInTitleOfWindow);
	}
}

void GUI_UpdateTime(void)
{
	static uint8_t minutePreviousValue = 255;
	static uint8_t hourPreviousValue = 255;

	//call update string functions only when it is necessary
	if((minutePreviousValue != ClockState.currentTimeMinute)
		||(hourPreviousValue != ClockState.currentTimeHour))
	{
		calculateTimeString(WidgetsStrings.labelMainWindowClockTimeValue, ClockState.currentTimeHour, ClockState.currentTimeMinute);
		UG_ButtonSetText(&mainWindow, BTN_ID_0, WidgetsStrings.labelMainWindowClockTimeValue);
	}

	minutePreviousValue = ClockState.currentTimeMinute;
	hourPreviousValue = ClockState.currentTimeHour;
}

void GUI_IncrementDay(uint8_t *day, uint8_t *month, uint8_t *year)
{
	(*day)++;
	if(*day > GUI_ReturnMaxDayInMonth(*month, *year))
	{
		*day = 1;
		(*month)++;

		if(*month > ((uint8_t)12))
		{
			*month = 1;
			(*year)++;
		}
	}
}

void GUI_DecrementDay(uint8_t *day, uint8_t *month, uint8_t *year)
{
	(*day)--;
	if(*day == 0)
	{
		(*month)--;
		if(*month == 0)
		{
			(*year)--;
			*month = 12;
		}

		*day = GUI_ReturnMaxDayInMonth(*month, *year);
	}
}

uint16_t GUI_GetIncrementedFramIndex(uint16_t value)
{
	if(value >= (MAX_RECORD_IN_FRAM - 1))
	{
		value = 0;
	}
	else
	{
		value++;
	}

	return value;
}

uint16_t GUI_GetDecrementedFramIndex(uint16_t value)
{
	if(value == 0)
	{
		value = (MAX_RECORD_IN_FRAM - 1);
	}
	else
	{
		value--;
	}

	return value;
}

uint16_t GUI_ReturnNewFramIndex(void)
{
	uint16_t nextIndexTmp = ClockState.currentFramIndex;

	//calculate new free FRAM index
	if(nextIndexTmp >= (MAX_RECORD_IN_FRAM - 1))
	{
		ClockState.currentFramIndex = 0;
	}
	else
	{
		ClockState.currentFramIndex++;
	}

	return nextIndexTmp;
}

void GUI_InitTemperatureStructure(uint8_t source, uint8_t day, uint8_t month, uint8_t year, TemperatureSingleDayRecordType *pointerToStructure)
{
	pointerToStructure->source = source;
	pointerToStructure->day = day;
	pointerToStructure->month = month;
	pointerToStructure->year = year;

	//fill temperatures record using invalid temperature value
	for(uint16_t i = 0; i < MAX_TEMP_RECORD_PER_DAY; i++)
	{
		pointerToStructure->temperatureValues[i] = INVALID_READ_SENSOR_VALUE;
	}
}

static void GUI_DrawTemperatureGraph(void)
{
	//end of table will represent right side
	uint16_t measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS];
	uint8_t measurementsTimestampDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS];
	TemperatureSingleDayRecordType* measurementsFramBlockTablePointer[NUM_OF_FRAM_BLOCK_IN_TABLE];
	uint8_t numberOfFramBlockInTablePointer = 0;

	uint16_t cursorMeasurementPosition = CURSOR_POSITION_ON_X_AXIS;
	uint16_t cursorInSingleStructure = 0;
	uint16_t copiedDataCounter = 0;

	ReadFramTempBufferType *pointerToStructureTmp = 0;

	uint16_t minTemperature = INVALID_INIT_TEMPERTAURE;//value is much higher than possible to measure
	uint16_t maxTemperature = 0;
	uint16_t xAxisHeigh = 0;//value which will be used to draw x axis (0 mean top, max mean down)
	uint16_t xValue = 0;//absolute max value chose from minTemperature and maxTemperature
	uint16_t yPixels = 0;
	uint16_t valuePerPixel = 0;//value used for calculation height of temperature point(value is multiply 10 times)
	uint16_t percentPosition = 0;
	uint16_t timestampStringPosition = 0;
	uint16_t temperatureCursorStringPositionY = 0;

	UG_AREA areaInsideTempWindow;
	UG_WindowGetArea(&temperatureWindow, &areaInsideTempWindow);

	areaInsideTempWindow.xs += TEMPERATURE_GRAPH_X_START;
	areaInsideTempWindow.xe = areaInsideTempWindow.xs + TEMPERATURE_GRAPH_WIDH;
	areaInsideTempWindow.ys += TEMPERATURE_GRAPH_Y_START;
	areaInsideTempWindow.ye = areaInsideTempWindow.ys + TEMPERATURE_GRAPH_HEIGH;

	for(uint16_t i = 0; i < NUM_OF_MEASUREMENTS_IN_X_AXIS; i++)
	{
		measurementsValueDataTable[i] = INVALID_READ_SENSOR_VALUE;
	}

	/**********************************
	*	gather data to draw
	***********************************/
	//init data under cursor
	pointerToStructureTmp = BufferCursor.structPointer;
	cursorInSingleStructure = BufferCursor.structIndex;

	measurementsFramBlockTablePointer[0] = &pointerToStructureTmp->singleRecord;
	numberOfFramBlockInTablePointer++;

	//copy data from right side and increment counter
	for(copiedDataCounter = 0; copiedDataCounter < NUM_OF_MEASUREMENTS_ON_RIGHT_SIDE + 1; )
	{
		measurementsValueDataTable[CURSOR_POSITION_ON_X_AXIS + copiedDataCounter]
		    = pointerToStructureTmp->singleRecord.temperatureValues[cursorInSingleStructure];
		measurementsTimestampDataTable[CURSOR_POSITION_ON_X_AXIS + copiedDataCounter] = cursorInSingleStructure;

		cursorInSingleStructure++;
		copiedDataCounter++;

		if((cursorInSingleStructure >= (MAX_TEMP_RECORD_PER_DAY))
			&& (pointerToStructureTmp->framIndex != ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].temperatureFramIndex))
		{
			pointerToStructureTmp = (ReadFramTempBufferType*)pointerToStructureTmp->pointerToNextElement;
			cursorInSingleStructure = 0;

			measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer] = &pointerToStructureTmp->singleRecord;
			numberOfFramBlockInTablePointer++;
		}
		//check left side border
		else if((pointerToStructureTmp->framIndex == ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].temperatureFramIndex)
			&& (cursorInSingleStructure >= (((ClockState.currentTimeHour*60) + ClockState.currentTimeMinute) / 15)))
		{
			break;
		}
	}

	//if less data was copied then half index then shift all including border
	if(copiedDataCounter < NUM_OF_MEASUREMENTS_ON_RIGHT_SIDE)
	{
		memmove(&measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - copiedDataCounter], &measurementsValueDataTable[CURSOR_POSITION_ON_X_AXIS], copiedDataCounter*2);
		memmove(&measurementsTimestampDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - copiedDataCounter], &measurementsTimestampDataTable[CURSOR_POSITION_ON_X_AXIS], copiedDataCounter);
	}

	pointerToStructureTmp = BufferCursor.structPointer;
	cursorInSingleStructure = BufferCursor.structIndex;
	copiedDataCounter--;

	//init cursor
	cursorMeasurementPosition = copiedDataCounter;

	//copy data from left side
	for(; copiedDataCounter < NUM_OF_MEASUREMENTS_IN_X_AXIS; )
	{
		measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - copiedDataCounter]
		    = pointerToStructureTmp->singleRecord.temperatureValues[cursorInSingleStructure];
		measurementsTimestampDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - copiedDataCounter] = cursorInSingleStructure;

		copiedDataCounter++;

		//don't execute this code in last iteration
		if((copiedDataCounter < NUM_OF_MEASUREMENTS_IN_X_AXIS) && cursorInSingleStructure == 0)
		{
			// if right side border will be detected then stop
			if((((ReadFramTempBufferType*)pointerToStructureTmp->pointerToPreviousElement)->availabilityFlag == false)
			 || (((ReadFramTempBufferType*)pointerToStructureTmp->pointerToPreviousElement)->notExistFlag == true))
			{
				break;
			}
			else
			{
				pointerToStructureTmp = (ReadFramTempBufferType*)pointerToStructureTmp->pointerToPreviousElement;
				cursorInSingleStructure = (MAX_TEMP_RECORD_PER_DAY - 1);

				memmove(&measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer], &measurementsFramBlockTablePointer[0], 4*numberOfFramBlockInTablePointer);
				measurementsFramBlockTablePointer[0] = &pointerToStructureTmp->singleRecord;
				numberOfFramBlockInTablePointer++;
			}
		}
		else
		{
			cursorInSingleStructure--;
		}
	}

	//find min and max temperature value in gathered data
	for(uint16_t i = 0; i < NUM_OF_MEASUREMENTS_IN_X_AXIS; i++)
	{
		if(measurementsValueDataTable[i] != INVALID_READ_SENSOR_VALUE)
		{
			if(minTemperature > measurementsValueDataTable[i])
				minTemperature = measurementsValueDataTable[i];

			if(maxTemperature < measurementsValueDataTable[i])
				maxTemperature = measurementsValueDataTable[i];
		}
	}

	//verify that min temperature value was initialized correctly(case when all data was gathered without temp sensor - is ivalid)
	if(minTemperature == INVALID_INIT_TEMPERTAURE)
	{
		//enter to this condition mean that min wasn't initiated correctly so assign default parameters
		maxTemperature = 100 + TEMPERATURE_OFFSET_FROM_ZERO;
		minTemperature = TEMPERATURE_OFFSET_FROM_ZERO - 10;
	}
	//verify that min temperature and max temperature isn't too small
	else if(false)
	{

	}

	//using min and max temperature value decide about graph schematic
	//case when all data is above zero
	if(minTemperature > TEMPERATURE_OFFSET_FROM_ZERO)
	{
		xAxisHeigh = TEMPERATURE_GRAPH_HEIGH - 1 - X_AXIS_DISTANCE_FROM_FRAME;
		percentPosition = 100;
	}
	//case when all data is below zero
	else if(maxTemperature < TEMPERATURE_OFFSET_FROM_ZERO)
	{
		xAxisHeigh = X_AXIS_DISTANCE_FROM_FRAME;
		percentPosition = 0;
	}
	else
	{
		percentPosition = ((maxTemperature - TEMPERATURE_OFFSET_FROM_ZERO)*100)
			/ ((maxTemperature - TEMPERATURE_OFFSET_FROM_ZERO) + (TEMPERATURE_OFFSET_FROM_ZERO - minTemperature));

		xAxisHeigh = (percentPosition*TEMPERATURE_GRAPH_HEIGH)/100;

		if(xAxisHeigh < X_AXIS_DISTANCE_FROM_FRAME)
			xAxisHeigh = X_AXIS_DISTANCE_FROM_FRAME;

		if(xAxisHeigh > (TEMPERATURE_GRAPH_HEIGH - 1 - X_AXIS_DISTANCE_FROM_FRAME))
			xAxisHeigh = TEMPERATURE_GRAPH_HEIGH - 1 - X_AXIS_DISTANCE_FROM_FRAME;
	}

	//if above half of y axis
	if(percentPosition > 50)
	{
		//xValue = maxTemperature;//verify
		xValue = maxTemperature - TEMPERATURE_OFFSET_FROM_ZERO;
		yPixels = xAxisHeigh;
		timestampStringPosition = 1;
		temperatureCursorStringPositionY = 11;
	}
	else
	{
		//xValue = minTemperature;
		xValue = TEMPERATURE_OFFSET_FROM_ZERO - minTemperature;
		yPixels = TEMPERATURE_GRAPH_HEIGH - xAxisHeigh;
		timestampStringPosition = TEMPERATURE_GRAPH_HEIGH - 8;
		temperatureCursorStringPositionY = TEMPERATURE_GRAPH_HEIGH - 20;
	}

	//calculate value which will be used to calculate distance from x axis(12 mean multiply 10 times + 20 percent)
	valuePerPixel = (xValue*12) / yPixels;

	/**********************************
	*	draw graph using gathered data
	***********************************/
	/**********************************
	*	init draw field
	***********************************/
	UG_FillFrame(areaInsideTempWindow.xs, areaInsideTempWindow.ys,
		areaInsideTempWindow.xe, areaInsideTempWindow.ye, C_WHITE_SMOKE);

	//sets default colors for fonts
	UG_SetForecolor(C_BLACK);
	UG_SetBackcolor(C_WHITE_SMOKE);

	/**********************************
	*	draw blue lines and timestamp
	***********************************/
	{
		uint8_t timestampString[12] =  { '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0' };

		UG_SetForecolor(C_BLUE);
		UG_FontSelect(&FONT_6X8);

		//draw blue lines and timestamp
		for(uint16_t i = 0, j = 1, lastZeroOccurence = 0; i < copiedDataCounter; i++)
		{
			if(measurementsTimestampDataTable[copiedDataCounter - i - 1] == (MAX_TEMP_RECORD_PER_DAY - 1))
			{
				//draw blue line
				UG_FillFrame(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ys,
					areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ye, C_BLUE);

				if((i - lastZeroOccurence) > DRAW_THRESHOLD_TIMESTAMP)
				{
					//draw timestamp
					calculateCalendarString(timestampString, measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer - j]->day,
						measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer - j]->month,
						measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer - j]->year, ".");

					UG_PutString(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH) + 1, areaInsideTempWindow.ys + timestampStringPosition, timestampString);
				}

				lastZeroOccurence = i;
				j++;
			}
			else if((i >= (copiedDataCounter - 1)) && ((NUM_OF_MEASUREMENTS_IN_X_AXIS - lastZeroOccurence) >= DRAW_THRESHOLD_TIMESTAMP)
				&& (i >= DRAW_THRESHOLD_TIMESTAMP))
			{
				//draw timestamp
				calculateCalendarString(timestampString, measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer - j]->day,
					measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer - j]->month,
					measurementsFramBlockTablePointer[numberOfFramBlockInTablePointer - j]->year, ".");

				UG_PutString(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH) + 1, areaInsideTempWindow.ys + timestampStringPosition, timestampString);

				break;
			}
		}/* for(uint16_t i = 0, j = 1, lastZeroOccurence = 0; i < copiedDataCounter; i++) */
	}

	/**********************************
	*	draw red cursor
	***********************************/
	{
		uint8_t stringTemperatureCursor[10] = {'\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'};

		//set string color
		UG_SetForecolor(C_BLACK);

		//draw cursor line
		UG_FillFrame(areaInsideTempWindow.xe - (cursorMeasurementPosition*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ys,
			areaInsideTempWindow.xe - (cursorMeasurementPosition*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ye, C_RED);

		calculateTemperatureString(stringTemperatureCursor, measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - cursorMeasurementPosition - 1], false);
		memmove(&stringTemperatureCursor[2], &stringTemperatureCursor[0], 5);

		stringTemperatureCursor[0] = 't';
		stringTemperatureCursor[1] = '=';

		//draw temperature string
		if(cursorMeasurementPosition > 16)
		{
			UG_PutString(areaInsideTempWindow.xe - (cursorMeasurementPosition*X_AXIS_ENTRIES_LENGTH) + 1,
				 areaInsideTempWindow.ys + temperatureCursorStringPositionY, stringTemperatureCursor);
		}
		else
		{
			UG_PutString(areaInsideTempWindow.xe - (cursorMeasurementPosition*X_AXIS_ENTRIES_LENGTH) - 55,
				areaInsideTempWindow.ys + temperatureCursorStringPositionY, stringTemperatureCursor);
		}
	}

	/**********************************
	*	draw x axis line with numbers
	***********************************/
	UG_FillFrame(areaInsideTempWindow.xs, areaInsideTempWindow.ys + xAxisHeigh,
			areaInsideTempWindow.xe, areaInsideTempWindow.ys + xAxisHeigh, C_BLACK);

	//set font which will be used to draw hours on x axis
	UG_FontSelect(&FONT_6X8);
	UG_SetForecolor(C_BLACK);

	//draw lines on x axis and hours
	for(uint16_t i = 0, j = 1; i < copiedDataCounter; i++)
	{
		//draw bigger line
		if(((measurementsTimestampDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i] + 1) % 4) == 0)
		{
			//draw long line on x axis
			UG_FillFrame(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ys + xAxisHeigh - 2,
				areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ys + xAxisHeigh + 2, C_BLACK);

			//draw hour on x axis
			if(j >= 1 && i > 3 && i < NUM_OF_MEASUREMENTS_IN_X_AXIS - 3)
			{
				uint8_t valueToDraw = (measurementsTimestampDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i] + 1)/(MAX_TEMP_RECORD_PER_DAY/24);
				uint8_t stringHours[3] = {'\0', '\0', '\0'};
				uint8_t drawNumberOffset = 3;

				//case to don't draw 24 as hour but 0
				if(valueToDraw == 24)
					valueToDraw = 0;

				//case when number use two digit in string
				if(valueToDraw > 9)
					drawNumberOffset *= 2;

				itoa(valueToDraw, stringHours, 10);

				//add rule to check x axis height
				if(xAxisHeigh < (TEMPERATURE_GRAPH_HEIGH - 13))
				{
					UG_PutString(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH) - drawNumberOffset, areaInsideTempWindow.ys + xAxisHeigh + 5,
						stringHours);
				}
				else
				{
					UG_PutString(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH) - drawNumberOffset, areaInsideTempWindow.ys + xAxisHeigh - 11,
						stringHours);
				}

				j = 0;
			}
			else
			{
				j++;
			}
		}
		else
		{
			//draw short line on x axis
			UG_FillFrame(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ys + xAxisHeigh - 1,
				areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), areaInsideTempWindow.ys + xAxisHeigh + 1, C_BLACK);
		}
	}

	/**********************************
	*	draw y scale on left side of draw area
	***********************************/
	{
		uint16_t xRoundValue = 0;
		uint16_t scalePositionValue = 0;
		uint16_t scalePositionPixels = 0;
		uint8_t scaleString[4] =  { '\0', '\0', '\0', '\0' };

		UG_SetForecolor(C_SILVER);

		if(xValue < 100)
		{
			xRoundValue = 10;
		}
		else
		{
			xRoundValue = 100;
		}

		scalePositionValue = (xValue/3)*2;

		scalePositionValue -= (scalePositionValue % xRoundValue);

		//draw scale above zero
		if((maxTemperature > TEMPERATURE_OFFSET_FROM_ZERO)
			&& (scalePositionValue < (maxTemperature - TEMPERATURE_OFFSET_FROM_ZERO)))
		{
			scalePositionPixels = (scalePositionValue*10)/valuePerPixel;
			scalePositionPixels = xAxisHeigh - scalePositionPixels;

			UG_FillFrame(areaInsideTempWindow.xs, areaInsideTempWindow.ys + scalePositionPixels,
				areaInsideTempWindow.xe, areaInsideTempWindow.ys + scalePositionPixels, C_SILVER);

			itoa(scalePositionValue/10, scaleString, 10);

			UG_PutString(areaInsideTempWindow.xs, areaInsideTempWindow.ys + scalePositionPixels - 3,
				scaleString);
		}

		//draw scale below zero
		if((minTemperature < TEMPERATURE_OFFSET_FROM_ZERO)
			&& (scalePositionValue < (TEMPERATURE_OFFSET_FROM_ZERO - minTemperature)))
		{
			scalePositionPixels = (scalePositionValue*10)/valuePerPixel;
			scalePositionPixels += xAxisHeigh;

			UG_FillFrame(areaInsideTempWindow.xs, areaInsideTempWindow.ys + scalePositionPixels,
				areaInsideTempWindow.xe, areaInsideTempWindow.ys + scalePositionPixels, C_SILVER);

			itoa(((int16_t)scalePositionValue/(int16_t)-10), scaleString, 10);

			UG_PutString(areaInsideTempWindow.xs, areaInsideTempWindow.ys + scalePositionPixels - 3,
				scaleString);
		}
	}

	/**********************************
	*	draw lines on graph
	***********************************/
	for(uint16_t i = 0; i < (copiedDataCounter - 1); i++)
	{
		if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i] != INVALID_READ_SENSOR_VALUE)
		{
			uint16_t point1Tmp = abs(((int16_t)(TEMPERATURE_OFFSET_FROM_ZERO)) - (int16_t)(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i]));
			point1Tmp = (point1Tmp*10)/valuePerPixel;

			if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i] == TEMPERATURE_OFFSET_FROM_ZERO)
				point1Tmp = xAxisHeigh;
			else if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i] > TEMPERATURE_OFFSET_FROM_ZERO)
				point1Tmp = xAxisHeigh - point1Tmp;
			else
				point1Tmp += xAxisHeigh;

			//draw line when two pint contain valid data
			if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 2 - i] != INVALID_READ_SENSOR_VALUE)
			{
				uint16_t point0Tmp = abs(((int16_t)(TEMPERATURE_OFFSET_FROM_ZERO)) - (int16_t)(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 2 - i]));
				point0Tmp = (point0Tmp*10)/valuePerPixel;

				if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 2 - i] == TEMPERATURE_OFFSET_FROM_ZERO)
					point0Tmp = xAxisHeigh;
				else if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 2 - i] > TEMPERATURE_OFFSET_FROM_ZERO)
					point0Tmp = xAxisHeigh - point0Tmp;
				else
					point0Tmp += xAxisHeigh;

				UG_DrawLine((areaInsideTempWindow.xe - ((i)*X_AXIS_ENTRIES_LENGTH)), (areaInsideTempWindow.ys + point1Tmp)
					, (areaInsideTempWindow.xe - ((i + 1)*X_AXIS_ENTRIES_LENGTH)), (areaInsideTempWindow.ys + point0Tmp),  C_RED);
			}

			//draw single dot when valid data is placed between two invalid data
			else if((i < (copiedDataCounter - 1)) && (i > 0)
				&& (measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 2 - i] == INVALID_READ_SENSOR_VALUE)
				&& (measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - i] == INVALID_READ_SENSOR_VALUE))
			{
				UG_FillFrame(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), (areaInsideTempWindow.ys + point1Tmp - 1),
					areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH), (areaInsideTempWindow.ys + point1Tmp + 1), C_RED);
				UG_FillFrame(areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH) - 1, (areaInsideTempWindow.ys + point1Tmp),
					areaInsideTempWindow.xe - (i*X_AXIS_ENTRIES_LENGTH) + 1, (areaInsideTempWindow.ys + point1Tmp), C_RED);
			}
		}/* if(measurementsValueDataTable[NUM_OF_MEASUREMENTS_IN_X_AXIS - 1 - i] != INVALID_READ_SENSOR_VALUE) */
	}/* for(uint16_t i = 0; i < (copiedDataCounter - 1); i++) */

	UG_FontSelect(NULL);
}

void GUI_ProcessTemperatureWindow(void)
{
	static UG_WINDOW* previousWindow = &mainWindow;
	static bool callRedrawFirstStructureFlag = true;
	static bool callRedrawSecondStructureFlag = true;

	//precondition to check that record is enabled
	if(ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].recordTemperature)
	{
		//case when temperature window was opened
		if((previousWindow == &mainWindow) && (gui.active_window == &temperatureWindow))
		{
			//clear flags in all structures
			for(uint16_t i = 0; i < READ_TEMP_FRAM_BUFFER_SIZE; i++)
			{
				ReadFramTempBufferTable[i].availabilityFlag = false;
				ReadFramTempBufferTable[i].notExistFlag = false;
			}

			//initialize first structure by present day temperature to buffer
			ReadFramTempBufferTable[0].singleRecord = TemperatureSingleDay[ClockState.temperatureTypeInWindow];
			ReadFramTempBufferTable[0].availabilityFlag = true;
			ReadFramTempBufferTable[0].framIndex = ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].temperatureFramIndex;
			ReadFramTempBufferTable[0].notExistFlag = false;

			//initialize cursor
			BufferCursor.lockMoveFlag = true;
			BufferCursor.source = ClockState.temperatureTypeInWindow;
			BufferCursor.structPointer = &ReadFramTempBufferTable[0];

			//calculate cursor default position - cursor point to last element of present day
			BufferCursor.structIndex = (((ClockState.currentTimeHour*60) + ClockState.currentTimeMinute) / 15);

			if(BufferCursor.structIndex != 0)
				BufferCursor.structIndex--;

			//set flag which will be used by second thread to inform that data must be gathered from FRAM
			BufferCursor.loadDataFlag = true;

			//initialize flag used to redraw on beginning
			callRedrawFirstStructureFlag = true;
			callRedrawSecondStructureFlag = true;
		}
		//case when temperature window is still open
		else if((previousWindow == &temperatureWindow) && (gui.active_window == &temperatureWindow))
		{
			bool callRedraw = false;

			//if decrement button was pressed recalculate cursor an set redraw flag
			if(graphLeftButtonWasPressed == true)
			{
				callRedraw = true;

				if(((int16_t)BufferCursor.structIndex - (int16_t)temperatureGraphStepValue) < 0)
				{
					//check that structure is available
					if(((ReadFramTempBufferType*)BufferCursor.structPointer->pointerToPreviousElement)->availabilityFlag)
					{
						BufferCursor.structPointer = ((ReadFramTempBufferType*)BufferCursor.structPointer->pointerToPreviousElement);
						BufferCursor.structIndex = MAX_TEMP_RECORD_PER_DAY - 1 - (uint8_t)(((int16_t)BufferCursor.structIndex - (int16_t)temperatureGraphStepValue)*(int16_t)-1) + 1;
					}
					else
					{
						if(BufferCursor.structIndex == 0)
						{
							callRedraw = false;
						}
						else
						{
							BufferCursor.structIndex = 0;
						}
					}
				}
				else
				{
					BufferCursor.structIndex -= temperatureGraphStepValue;
				}

				graphLeftButtonWasPressed = false;
			}/* if(graphLeftButtonWasPressed == true) */

			//if increment button was pressed recalculate cursor and set redraw flag
			if(graphRightButtonWasPressed == true)
			{
				callRedraw = true;
				//check that fram index is equal as current day fram index
				if(BufferCursor.structPointer->framIndex == ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].temperatureFramIndex)
				{
					uint16_t currentDayIndex = (((ClockState.currentTimeHour*60) + ClockState.currentTimeMinute) / 15);
					if(currentDayIndex != 0)
						currentDayIndex--;

					if(((uint16_t)BufferCursor.structIndex + (uint16_t)temperatureGraphStepValue) >= currentDayIndex)
					{
						if(BufferCursor.structIndex == currentDayIndex)
						{
							callRedraw = false;
						}
						else
						{
							BufferCursor.structIndex = currentDayIndex;
						}
					}
					else
					{
						BufferCursor.structIndex += temperatureGraphStepValue;
					}
				}
				else if(((uint16_t)BufferCursor.structIndex + (uint16_t)temperatureGraphStepValue) > (MAX_TEMP_RECORD_PER_DAY - 1))
				{
					BufferCursor.structPointer = ((ReadFramTempBufferType*)BufferCursor.structPointer->pointerToNextElement);
					BufferCursor.structIndex = ((uint16_t)BufferCursor.structIndex + (uint16_t)temperatureGraphStepValue) - (uint16_t)(MAX_TEMP_RECORD_PER_DAY);
				}
				else
				{
					BufferCursor.structIndex += temperatureGraphStepValue;
				}

				graphRightButtonWasPressed = false;
			}/* if(graphRightButtonWasPressed == true) */


			//call redraw on beginning when first two structure will be loaded (two because graph is too small for more)
			{
				//check redraw flag and availability of first structure
				if(callRedrawFirstStructureFlag && BufferCursor.structPointer->availabilityFlag)
				{
					callRedraw = true;

					callRedrawFirstStructureFlag = false;
				}

				//check redraw flag and availability of second structure
				if(callRedrawSecondStructureFlag &&
					((ReadFramTempBufferType*)BufferCursor.structPointer->pointerToPreviousElement)->availabilityFlag)
				{
					callRedraw = true;

					callRedrawSecondStructureFlag = false;
				}
			}

			//if redraw flag was set then draw graph
			if(callRedraw == true)
			{
				GUI_DrawTemperatureGraph();
			}
		}
		//case when temperature window was closed
		else if((previousWindow == &temperatureWindow) && (gui.active_window != &temperatureWindow))
		{
			//set flag which will be used by second thread to inform that load data from FRAM isn't necessary
			BufferCursor.loadDataFlag = false;
		}
	}/* if(ClockState.TemperatureSensorTable[ClockState.temperatureTypeInWindow].recordTemperature) */

	previousWindow = gui.active_window;
}

static void drawAllarmStatus(UG_AREA position, UG_COLOR firstCollor, UG_COLOR secondCollor, bool active)
{
	if(active)
	{
		UG_FillFrame(position.xs, position.ys, position.xe, position.ye, firstCollor);

		UG_FillFrame(position.xs + ALARM_BUTTON_BORDER_DISTANCE, position.ys + ALARM_BUTTON_BORDER_DISTANCE,
			position.xe - ALARM_BUTTON_BORDER_DISTANCE, position.ye - ALARM_BUTTON_BORDER_DISTANCE, secondCollor);
	}
	else
	{
		UG_FillFrame(position.xs, position.ys, position.xe, position.ye, DEFAULT_GUI_BACKGROUND_COLOR);
	}
}

void GUI_ProcessAlarmAnimation(void)
{
	static UG_WINDOW* previousWindow = &temperatureWindow;

	//draw animation of active alarm only when window was opened
	if((gui.active_window == &mainWindow) && (previousWindow != &mainWindow))
	{
		//get x axis position commnon for all button
		UG_WindowGetArea(&mainWindow, &possitionButtonFirstAlarm);
		possitionButtonFirstAlarm.xs += GRID_X1_MW;
		possitionButtonFirstAlarm.xe = possitionButtonFirstAlarm.xs + ALARM_BUTTON_LENGTH_MW - 1;

		possitionButtonSecondAlarm = possitionButtonFurnaceAlarm = possitionButtonFirstAlarm;

		//initialize y axis for all button separately
		possitionButtonFirstAlarm.ys += GRID_Y1_MW + 1;
		possitionButtonFirstAlarm.ye = possitionButtonFirstAlarm.ys + ALARM_BUTTON_HEIGH_MW - 2;

		possitionButtonSecondAlarm.ys += GRID_Y1_SECOND_BUTTON_MW + 1;
		possitionButtonSecondAlarm.ye = possitionButtonSecondAlarm.ys + ALARM_BUTTON_HEIGH_MW - 2;

		possitionButtonFurnaceAlarm.ys += GRID_Y1_THIRD_BUTTON_MW + 1;
		possitionButtonFurnaceAlarm.ye = possitionButtonFurnaceAlarm.ys + ALARM_BUTTON_HEIGH_MW - 2;

		//draw state of alarm in field
		drawAllarmStatus(possitionButtonFirstAlarm, C_BLACK, C_WHITE_SMOKE, ClockState.firstAlarmActive);
		drawAllarmStatus(possitionButtonSecondAlarm, C_BLACK, C_WHITE_SMOKE, ClockState.secondAlarmActive);
		drawAllarmStatus(possitionButtonFurnaceAlarm, C_DARK_ORANGE, C_GOLD, ClockState.temperatureFurnaceAlarmActive);
	}
	//if mainwindow is opened draw blinking when alarm is raised
	else if(gui.active_window == &mainWindow)
	{
		static bool alarmButtonState = true;
		static uint16_t lastValueOfAlarmAnimationStep = 0;

		if((lastValueOfAlarmAnimationStep != ClockState.currentTimeSecond)
			&& (ClockState.firstAlarmRaised || ClockState.secondAlarmRaised
			|| ClockState.temperatureFurnaceAlarmRaised))
		{
			alarmButtonState ^= true;
			lastValueOfAlarmAnimationStep = ClockState.currentTimeSecond;

			if(ClockState.firstAlarmRaised)
				drawAllarmStatus(possitionButtonFirstAlarm, C_BLACK, C_WHITE_SMOKE, alarmButtonState);

			if(ClockState.secondAlarmRaised)
				drawAllarmStatus(possitionButtonSecondAlarm, C_BLACK, C_WHITE_SMOKE, alarmButtonState);

			if(ClockState.temperatureFurnaceAlarmRaised)
				drawAllarmStatus(possitionButtonFurnaceAlarm, C_DARK_ORANGE, C_GOLD, alarmButtonState);
		}

		//case when allarm was disable and animation hand in inactive state
		if((alarmButtonState == false) && (ClockState.firstAlarmRaised == false)
			&& (ClockState.secondAlarmRaised == false) && (ClockState.temperatureFurnaceAlarmRaised == false))
		{
			alarmButtonState = true;
			drawAllarmStatus(possitionButtonFirstAlarm, C_BLACK, C_WHITE_SMOKE, ClockState.firstAlarmActive);
			drawAllarmStatus(possitionButtonSecondAlarm, C_BLACK, C_WHITE_SMOKE, ClockState.secondAlarmActive);
			drawAllarmStatus(possitionButtonFurnaceAlarm, C_DARK_ORANGE, C_GOLD, ClockState.temperatureFurnaceAlarmActive);
		}
	}

	previousWindow = gui.active_window;
}

void GUI_IncrementSecond(void)
{
	ClockState.systemUpTime++;
	ClockState.currentTimeSecond++;

	if(ClockState.currentTimeSecond >= 60)
	{
		ClockState.currentTimeSecond = 0;
		ClockState.currentTimeMinute++;

		if(ClockState.currentTimeMinute >= 60)
		{
			ClockState.currentTimeMinute = 0;
			ClockState.currentTimeHour++;

			if(ClockState.currentTimeHour >= 24)
			{
				ClockState.currentTimeHour = 0;

				//increment day if calendar day was set
				if(ClockState.day != INVALID_CALENDAR_DATE && ClockState.month != INVALID_CALENDAR_DATE
					&& ClockState.year != INVALID_CALENDAR_DATE)
				{
					GUI_IncrementDay(&ClockState.day, &ClockState.month, &ClockState.year);
				}
			}
		}
	}
}

void GUI_RefreshWifiWindow(void)
{
	if (gui.active_window == &wifiSettingsWindow)
	{
		//refresh Wifi APN list if case was fulfiled
		if(ClockState.wifiApnReceived || refreshWifiList)
		{
			//check correctness of cursor position
			if (wifiListCursorPosition > 0)
			{
				int8_t wifiListCursorPositionTmp = (int8_t)ApnStructure.NumberOfApn - (int8_t)wifiListCursorPosition;

				if (wifiListCursorPositionTmp < (int8_t)NUMBER_OF_WIFI_DISPLAY_NETWORKS)
				{
					wifiListCursorPositionTmp = (int8_t)ApnStructure.NumberOfApn - (int8_t)NUMBER_OF_WIFI_DISPLAY_NETWORKS;

					if (wifiListCursorPositionTmp < 0)
					{
						wifiListCursorPositionTmp = 0;
					}

					wifiListCursorPosition = wifiListCursorPositionTmp;
				}

			}/* if (wifiListCursorPosition > 0) */

			//refresh widgets with Wifi network
			for (int i = 0; i < NUMBER_OF_WIFI_DISPLAY_NETWORKS; i++)
			{
				if ((i + wifiListCursorPosition) < ApnStructure.NumberOfApn)
				{
					UG_TextboxSetText(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i),
							ApnStructure.AppInformationTable[i + wifiListCursorPosition].ssid);

					UG_TextboxShow(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i));
					UG_ButtonShow(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i));

					if(ClockState.wifiConnected &&
							(strcmp(ApnStructure.AppInformationTable[i + wifiListCursorPosition].ssid, ClockState.ssidOfAssignedApn) == 0))
					{
						UG_ButtonSetText(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i), "connected");
					}
					else
					{
						UG_ButtonSetText(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i), "connect");
					}
				}
				else
				{
					UG_TextboxHide(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i));
					UG_ButtonHide(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i));
				}
			}/* for (int i = 0; i < NUMBER_OF_WIFI_DISPLAY_NETWORKS; i++) */

			//update up scroll visibility
			if (wifiListCursorPosition == 0)
			{
				UG_ButtonHide(&wifiSettingsWindow, BTN_ID_8);
			}
			else
			{
				UG_ButtonShow(&wifiSettingsWindow, BTN_ID_8);
			}

			//update down scroll visibility
			if ((ApnStructure.NumberOfApn <= NUMBER_OF_WIFI_DISPLAY_NETWORKS)
					|| ((ApnStructure.NumberOfApn - wifiListCursorPosition - NUMBER_OF_WIFI_DISPLAY_NETWORKS) == 0))
			{
				UG_ButtonHide(&wifiSettingsWindow, BTN_ID_9);
			}
			else
			{
				UG_ButtonShow(&wifiSettingsWindow, BTN_ID_9);
			}

			UG_Update();

			ClockState.wifiApnReceived = false;
			refreshWifiList = false;
		}/* if(ClockState.wifiApnReceived || refreshWifiList) */

		//refresh Wifi status
		{
			static WIFI_GUI_STATUS wifiGuiStatus = WIFI_INACTIVE;
			WIFI_GUI_STATUS wifiGuiStatusTmp = WIFI_ACTIVE;

			if(ClockState.wifiReady)
			{
				wifiGuiStatusTmp = WIFI_ACTIVE;

				if(ClockState.wifiConnected)
				{
					wifiGuiStatusTmp = WIFI_CONNECTED;
				}
			}
			else
			{
				wifiGuiStatusTmp = WIFI_INACTIVE;
			}

			//refresh WiFi state
			if(wifiGuiStatus != wifiGuiStatusTmp)
			{
				wifiGuiStatus = wifiGuiStatusTmp;

				switch(wifiGuiStatus)
				{
				case WIFI_ACTIVE:
					strcpy(WidgetsStrings.labelWifiStatus, "Active");
					UG_ButtonHide(&wifiSettingsWindow, BTN_ID_1);
					UG_TextboxHide(&wifiSettingsWindow, TXB_ID_1);
					break;

				case WIFI_INACTIVE:
					strcpy(WidgetsStrings.labelWifiStatus, "Inactive");
					UG_ButtonHide(&wifiSettingsWindow, BTN_ID_1);
					UG_TextboxHide(&wifiSettingsWindow, TXB_ID_1);
					break;

				case WIFI_CONNECTED:
					strcpy(WidgetsStrings.labelWifiStatus, "Connected to: ");
					strcat(WidgetsStrings.labelWifiStatus, ClockState.ssidOfAssignedApn);
					UG_ButtonShow(&wifiSettingsWindow, BTN_ID_1);
					break;
				}

				UG_TextboxSetText(&wifiSettingsWindow, TXB_ID_0, WidgetsStrings.labelWifiStatus);
			}
		}//refresh Wifi status

		//refresh Wifi IP adress
		{
			static bool previousWifiConnectedState = false;
			static uint8_t previousAssignedFirstByteOfIp = 0;

			if((ClockState.wifiConnected && (previousWifiConnectedState != ClockState.wifiConnected))
				|| (ClockState.wifiConnected && (previousAssignedFirstByteOfIp != ClockState.ipAddressAssignedToDevice[0])))
			{
				uint8_t numberTmp[5] = { '\0', '\0', '\0', '\0', '\0' };

				strcpy(WidgetsStrings.labelAssignedIpAdress, "IP: ");

				if(ClockState.ipAddressAssignedToDevice[0] != 0)
				{
					for(int i = 0; i < IP_ADDRESS_BYTE_LENGTH; i++)
					{
						itoa(ClockState.ipAddressAssignedToDevice[i], numberTmp, 10);
						strcat(WidgetsStrings.labelAssignedIpAdress, numberTmp);
						strcat(WidgetsStrings.labelAssignedIpAdress, ".");
					}

					//delete last dot in IP address
					WidgetsStrings.labelAssignedIpAdress[strlen(WidgetsStrings.labelAssignedIpAdress) - 1] = '\0';
				}

				UG_TextboxSetText(&wifiSettingsWindow, TXB_ID_1, WidgetsStrings.labelAssignedIpAdress);
				UG_TextboxShow(&wifiSettingsWindow, TXB_ID_1);
			}

			previousWifiConnectedState = ClockState.wifiConnected;
			previousAssignedFirstByteOfIp = ClockState.ipAddressAssignedToDevice[0];
		}//refresh Wifi IP adresss
	}/* if (gui.active_window == &wifiSettingsWindow) */
}

void GUI_ClockInit(void)
{
	/**********************************
	* init string inside ClocState structure
	***********************************/
	calculateTemperatureString(WidgetsStrings.labelTemperatureInside,
		ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValue, true);
	calculateTemperatureString(WidgetsStrings.labelTemperatureOutside,
		ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValue, true);
	calculateTemperatureString(WidgetsStrings.labelTemperatureFurnace,
		ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue, true);

	strcpy(WidgetsStrings.labelTemperatureInTitleOfWindow, " ");

	/**********************************
	* init ugui and register driver
	***********************************/
#ifndef MICROCONTROLLER
#if 1//draw on SDL layer
	UG_Init(&gui, SDL_LCD_PixelSetFunction, 320, 240);

	//Activate driver
	UG_DriverRegister(DRIVER_FILL_FRAME, (void*)SDL_LCD_FillFrame);
	UG_DriverEnable(DRIVER_FILL_FRAME);
#else//draw on real LCD
	UG_Init(&gui, LCD_SetPixel_uGui, 320, 240);

	//Activate driver
	UG_DriverRegister(DRIVER_FILL_FRAME, (void*)LCD_FillFrame_uGui);
	UG_DriverEnable(DRIVER_FILL_FRAME);
#endif
#else
	UG_Init(&gui, LCD_SetPixel_uGui, 320, 240);

	//Activate drivers
	UG_DriverRegister(DRIVER_FILL_FRAME, (void*)LCD_FillFrame_uGui);
	UG_DriverEnable(DRIVER_FILL_FRAME);

	UG_DriverRegister(DRIVER_PIXEL_IN_AREA_START, (void*)LCD_StartFillArea_uGui);
	UG_DriverEnable(DRIVER_PIXEL_IN_AREA_START);

	UG_DriverRegister(DRIVER_PIXEL_IN_AREA_PUT, (void*)LCD_PixelFillArea_uGui);
	UG_DriverEnable(DRIVER_PIXEL_IN_AREA_PUT);

	UG_DriverRegister(DRIVER_PIXEL_IN_AREA_STOP, (void*)LCD_StopFillArea_uGui);
	UG_DriverEnable(DRIVER_PIXEL_IN_AREA_STOP);
#endif
	/**********************************
	* Create the main window
	***********************************/
	UG_WindowCreate(&mainWindow, mainWindowObjects, MAX_OBJECTS, mainWindowHandler);
	/* Modify the window title */
	UG_WindowSetTitleHeight(&mainWindow, 0);

	UG_ButtonCreate(&mainWindow, &buttonClockValue, BTN_ID_0, GRID_X1_MW + 30, GRID_Y1_MW, 284, GRID_Y1_MW + 79);
	UG_ButtonSetFont(&mainWindow, BTN_ID_0, &FONT_32X53);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_0, BTN_STYLE_2D);
	UG_ButtonSetAlignment(&mainWindow, BTN_ID_0, ALIGN_CENTER);

	GUI_UpdateTime();

	UG_ButtonCreate(&mainWindow, &buttonClockValue, BTN_ID_0, GRID_X1_MW + 30, GRID_Y1_MW, 284, GRID_Y1_MW + 79);
	UG_ButtonSetFont(&mainWindow, BTN_ID_0, &FONT_32X53);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_0, BTN_STYLE_2D);
	UG_ButtonSetAlignment(&mainWindow, BTN_ID_0, ALIGN_CENTER);
	calculateTimeString(WidgetsStrings.labelMainWindowClockTimeValue, ClockState.currentTimeHour, ClockState.currentTimeMinute);
	UG_ButtonSetText(&mainWindow, BTN_ID_0, WidgetsStrings.labelMainWindowClockTimeValue);

	UG_ButtonCreate(&mainWindow, &buttonSettings, BTN_ID_1, 290, 0, 311, 20);

	UG_TextboxCreate(&mainWindow, &textBoxTemperatureOutside, TXB_ID_1, GRID_X2_MW, GRID_Y2_MW, GRID_X2_MW + LABEL_LENGTH, GRID_Y2_MW + PICTURE_HEIGHT);
	UG_TextboxSetAlignment(&mainWindow, TXB_ID_1, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&mainWindow, TXB_ID_1, WidgetsStrings.labelTemperatureOutside);
	UG_TextboxSetFont(&mainWindow, TXB_ID_1, &FONT_10X16);

	UG_TextboxCreate(&mainWindow, &textBoxTemperatureInside, TXB_ID_2, GRID_X2_MW, GRID_Y3_MW, GRID_X2_MW + LABEL_LENGTH, GRID_Y3_MW + PICTURE_HEIGHT);
	UG_TextboxSetAlignment(&mainWindow, TXB_ID_2, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&mainWindow, TXB_ID_2, WidgetsStrings.labelTemperatureInside);
	UG_TextboxSetFont(&mainWindow, TXB_ID_2, &FONT_10X16);

	UG_TextboxCreate(&mainWindow, &textBoxTemperatureFurnace, TXB_ID_3, GRID_X4_MW, GRID_Y2_MW, GRID_X4_MW + LABEL_LENGTH, GRID_Y2_MW + PICTURE_HEIGHT);
	UG_TextboxSetAlignment(&mainWindow, TXB_ID_3, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&mainWindow, TXB_ID_3, WidgetsStrings.labelTemperatureFurnace);
	UG_TextboxSetFont(&mainWindow, TXB_ID_3, &FONT_10X16);

	UG_ButtonCreate(&mainWindow, &buttonTemperatureOutside, BTN_ID_2, GRID_X1_MW - 1, GRID_Y2_MW - 1, GRID_X1_MW + PICTURE_LENGTH, GRID_Y2_MW + PICTURE_HEIGHT);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_2, BTN_STYLE_2D);

	UG_ButtonCreate(&mainWindow, &buttonTemperatureInside, BTN_ID_3, GRID_X1_MW - 1, GRID_Y3_MW - 1, GRID_X1_MW + PICTURE_LENGTH, GRID_Y3_MW + PICTURE_HEIGHT);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_3, BTN_STYLE_2D);

	UG_ButtonCreate(&mainWindow, &buttonTemperatureFurnace, BTN_ID_4, GRID_X3_MW - 1, GRID_Y2_MW - 1, GRID_X3_MW + PICTURE_LENGTH, GRID_Y2_MW + PICTURE_HEIGHT);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_4, BTN_STYLE_2D);

	UG_ButtonCreate(&mainWindow, &buttonWiFiSettings, BTN_ID_5, GRID_X3_MW - 1, GRID_Y3_MW - 1, GRID_X3_MW + PICTURE_LENGTH, GRID_Y3_MW + PICTURE_HEIGHT);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_5, BTN_STYLE_2D);

	UG_ImageCreate(&mainWindow, &imageTemperatureOutside, IMG_ID_0, GRID_X1_MW, GRID_Y2_MW, 0, 0);
	UG_ImageSetBMP(&mainWindow, IMG_ID_0, &homePictureOutside);

	UG_ImageCreate(&mainWindow, &imageTemperatureInside, IMG_ID_1, GRID_X1_MW, GRID_Y3_MW, 0, 0);
	UG_ImageSetBMP(&mainWindow, IMG_ID_1, &homePictureInside);

	UG_ImageCreate(&mainWindow, &imageTemperatureFurnace, IMG_ID_2, GRID_X3_MW, GRID_Y2_MW, 0, 0);
	UG_ImageSetBMP(&mainWindow, IMG_ID_2, &furnacePicture);

	UG_ImageCreate(&mainWindow, &imageWiFiSettings, IMG_ID_3, GRID_X3_MW, GRID_Y3_MW, 0, 0);
	UG_ImageSetBMP(&mainWindow, IMG_ID_3, &wifiPicture);

	UG_ButtonCreate(&mainWindow, &buttonFirstAllarmStatus, BTN_ID_6, GRID_X1_MW - 1, GRID_Y1_MW, GRID_X1_MW + ALARM_BUTTON_LENGTH_MW, GRID_Y1_MW + ALARM_BUTTON_HEIGH_MW);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_6, BTN_STYLE_2D);

	UG_ButtonCreate(&mainWindow, &buttonSecondAllarmStatus, BTN_ID_7, GRID_X1_MW - 1, GRID_Y1_SECOND_BUTTON_MW,
		GRID_X1_MW + ALARM_BUTTON_LENGTH_MW, GRID_Y1_SECOND_BUTTON_MW + ALARM_BUTTON_HEIGH_MW);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_7, BTN_STYLE_2D);

	UG_ButtonCreate(&mainWindow, &buttonFurnaceAllarmStatus, BTN_ID_8, GRID_X1_MW - 1, GRID_Y1_THIRD_BUTTON_MW,
		GRID_X1_MW + ALARM_BUTTON_LENGTH_MW, GRID_Y1_THIRD_BUTTON_MW + ALARM_BUTTON_HEIGH_MW);
	UG_ButtonSetStyle(&mainWindow, BTN_ID_8, BTN_STYLE_2D);

	/**********************************
	* Create the clock setting window
	***********************************/
	UG_WindowCreate(&clockSettingsWindow, clockSettingsObjects, MAX_OBJECTS, clockSettingsWindowHandler);
	UG_WindowSetTitleText(&clockSettingsWindow, "clock settings");
	UG_WindowSetTitleTextFont(&clockSettingsWindow, &FONT_8X8);

	UG_TextboxCreate(&clockSettingsWindow, &textBoxSettingInformation, TXB_ID_0, 0, 0, 270, 20);
	UG_TextboxSetAlignment(&clockSettingsWindow, TXB_ID_0, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&clockSettingsWindow, TXB_ID_0, "actual time:");
	UG_TextboxSetFont(&clockSettingsWindow, TXB_ID_0, &FONT_10X16);

	UG_TextboxCreate(&clockSettingsWindow, &textBoxTimeValue, TXB_ID_1, 20, GRID_Y1_CSW + SET_TIME_BUTTON_HEIGHT + 2, 290, GRID_Y1_CSW + SET_TIME_BUTTON_HEIGHT + 60);
	UG_TextboxSetAlignment(&clockSettingsWindow, TXB_ID_1, ALIGN_CENTER);
	UG_TextboxSetText(&clockSettingsWindow, TXB_ID_1, " ");
	UG_TextboxSetFont(&clockSettingsWindow, TXB_ID_1, &FONT_32X53);

	UG_ButtonCreate(&clockSettingsWindow, &buttonIncrementHour, BTN_ID_0, GRID_X1_CSW, GRID_Y1_CSW, GRID_X1_CSW + SET_TIME_BUTTON_LENGTH, GRID_Y1_CSW + SET_TIME_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_0, BTN_STYLE_2D);

	UG_ButtonCreate(&clockSettingsWindow, &buttonDecrementHour, BTN_ID_1, GRID_X1_CSW, GRID_Y2_CSW, GRID_X1_CSW + SET_TIME_BUTTON_LENGTH, GRID_Y2_CSW + SET_TIME_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_1, BTN_STYLE_2D);

	UG_ButtonCreate(&clockSettingsWindow, &buttonIncrementMinute, BTN_ID_2, GRID_X2_CSW, GRID_Y1_CSW, GRID_X2_CSW + SET_TIME_BUTTON_LENGTH, GRID_Y1_CSW + SET_TIME_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_2, BTN_STYLE_2D);

	UG_ButtonCreate(&clockSettingsWindow, &buttonDecrementMinute, BTN_ID_3, GRID_X2_CSW, GRID_Y2_CSW, GRID_X2_CSW + SET_TIME_BUTTON_LENGTH, GRID_Y2_CSW + SET_TIME_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_3, BTN_STYLE_2D);

	UG_ButtonCreate(&clockSettingsWindow, &buttonChoseTimeValue, BTN_ID_4, 1, 160, 100, 210);
	UG_ButtonSetText(&clockSettingsWindow, BTN_ID_4, "time");
	UG_ButtonSetFont(&clockSettingsWindow, BTN_ID_4, &FONT_8X12);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_4, BTN_STYLE_2D);

	UG_ButtonCreate(&clockSettingsWindow, &buttonChoseFirstAlarm, BTN_ID_5, 103, 160, 203, 188);
	UG_ButtonSetText(&clockSettingsWindow, BTN_ID_5, "1st alarm");
	UG_ButtonSetFont(&clockSettingsWindow, BTN_ID_5, &FONT_8X12);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_5, BTN_STYLE_2D);

	UG_ButtonCreate(&clockSettingsWindow, &buttonChoseSecondAlarm, BTN_ID_6, 206, 160, 306, 188);
	UG_ButtonSetText(&clockSettingsWindow, BTN_ID_6, "2nd alarm");
	UG_ButtonSetFont(&clockSettingsWindow, BTN_ID_6, &FONT_8X12);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_6, BTN_STYLE_2D);

	UG_CheckboxCreate(&clockSettingsWindow, &ckeckBoxActiveFirstAlarm, CHB_ID_0, 103, 190, 203, 210);
	UG_CheckboxSetText(&clockSettingsWindow, CHB_ID_0, "active");
	UG_CheckboxSetFont(&clockSettingsWindow, CHB_ID_0, &FONT_8X12);
	if (ClockState.firstAlarmActive)
		UG_CheckboxSetCheched(&clockSettingsWindow, CHB_ID_0, CHB_STATE_PRESSED);
	else
		UG_CheckboxSetCheched(&clockSettingsWindow, CHB_ID_0, CHB_STATE_RELEASED);

	UG_CheckboxCreate(&clockSettingsWindow, &ckeckBoxActiveSecondAlarm, CHB_ID_1, 206, 190, 306, 210);
	UG_CheckboxSetText(&clockSettingsWindow, CHB_ID_1, "active");
	UG_CheckboxSetFont(&clockSettingsWindow, CHB_ID_1, &FONT_8X12);
	if (ClockState.secondAlarmActive)
		UG_CheckboxSetCheched(&clockSettingsWindow, CHB_ID_1, CHB_STATE_PRESSED);
	else
		UG_CheckboxSetCheched(&clockSettingsWindow, CHB_ID_1, CHB_STATE_RELEASED);

	UG_ButtonCreate(&clockSettingsWindow, &buttonCloseClockSettingsWindow, BTN_ID_7, 290, 0, 311, 20);
	UG_ButtonSetStyle(&clockSettingsWindow, BTN_ID_7, BTN_STYLE_2D);

	/**********************************
	* Create the temperature window
	***********************************/
	UG_WindowCreate(&temperatureWindow, temperatureWindowObjects, MAX_OBJECTS, temperatureWindowHandler);
	UG_WindowSetTitleText(&temperatureWindow, "temperature");
	UG_WindowSetTitleTextFont(&temperatureWindow, &FONT_8X8);

	UG_TextboxCreate(&temperatureWindow, &textBoxTemperatureInformation, TXB_ID_0, 0, 0, 289, 20);
	UG_TextboxSetAlignment(&temperatureWindow, TXB_ID_0, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&temperatureWindow, TXB_ID_0, WidgetsStrings.labelTemperatureInTitleOfWindow);
	UG_TextboxSetFont(&temperatureWindow, TXB_ID_0, &FONT_8X12);

	UG_TextboxCreate(&temperatureWindow, &textBoxGraphLabel, TXB_ID_1, 0, 20, 310, 40);
	UG_TextboxSetAlignment(&temperatureWindow, TXB_ID_1, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&temperatureWindow, TXB_ID_1, "history graph:");
	UG_TextboxSetFont(&temperatureWindow, TXB_ID_1, &FONT_8X12);

	UG_ButtonCreate(&temperatureWindow, &buttonHistoryGraphLeft, BTN_ID_0, 5, 40, 30, 140);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_0, BTN_STYLE_2D);

	UG_ButtonCreate(&temperatureWindow, &buttonHistoryGraphRight, BTN_ID_1, 280, 40, 305, 140);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_1, BTN_STYLE_2D);

	UG_CheckboxCreate(&temperatureWindow, &ckeckBoxRecordTemperature, CHB_ID_1, 1, 142, 180, 158);
	UG_CheckboxSetFont(&temperatureWindow, CHB_ID_1, &FONT_6X8);
	UG_CheckboxSetCheched(&temperatureWindow, CHB_ID_1, CHB_STATE_RELEASED);
	UG_CheckboxSetText(&temperatureWindow, CHB_ID_1, "record temperature");

	UG_TextboxCreate(&temperatureWindow, &textBoxTemperatureOffset, TXB_ID_2, 0, GRID_Y2_TW, 149, GRID_Y2_TW + 12);
	UG_TextboxSetAlignment(&temperatureWindow, TXB_ID_2, ALIGN_CENTER_LEFT);
	UG_TextboxSetFont(&temperatureWindow, TXB_ID_2, &FONT_6X8);
	calculateTemperatureOffsetString();
	UG_TextboxSetText(&temperatureWindow, TXB_ID_2, WidgetsStrings.labelTemperatureFurnaceOffset);

	UG_CheckboxCreate(&temperatureWindow, &ckeckBoxTemperatureAlarm, CHB_ID_0, 160, GRID_Y2_TW, 310, GRID_Y2_TW + 12);
	UG_CheckboxSetFont(&temperatureWindow, CHB_ID_0, &FONT_6X8);
	UG_CheckboxSetCheched(&temperatureWindow, CHB_ID_0, CHB_STATE_RELEASED);
	calculateTemperatureAlarmString();
	UG_CheckboxSetText(&temperatureWindow, CHB_ID_0, WidgetsStrings.labelTemperatureFurnaceAlarm);

	UG_ButtonCreate(&temperatureWindow, &buttonIncrementTemperatureOffset, BTN_ID_2, GRID_X1_TW, GRID_Y1_TW, GRID_X1_TW + SET_TEMPERATURE_BUTTON_LENGTH, GRID_Y1_TW + SET_TEMPERATURE_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_2, BTN_STYLE_2D);

	UG_ButtonCreate(&temperatureWindow, &buttonDecrementTemperatureOffset, BTN_ID_3, GRID_X1_TW, GRID_Y3_TW, GRID_X1_TW + SET_TEMPERATURE_BUTTON_LENGTH, GRID_Y3_TW + SET_TEMPERATURE_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_3, BTN_STYLE_2D);

	UG_ButtonCreate(&temperatureWindow, &buttonIncrementTemperatureAlarm, BTN_ID_4, GRID_X2_TW, GRID_Y1_TW, GRID_X2_TW + SET_TEMPERATURE_BUTTON_LENGTH, GRID_Y1_TW + SET_TEMPERATURE_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_4, BTN_STYLE_2D);

	UG_ButtonCreate(&temperatureWindow, &buttonDecrementTemperatureAlarm, BTN_ID_5, GRID_X2_TW, GRID_Y3_TW, GRID_X2_TW + SET_TEMPERATURE_BUTTON_LENGTH, GRID_Y3_TW + SET_TEMPERATURE_BUTTON_HEIGHT);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_5, BTN_STYLE_2D);

	UG_ButtonCreate(&temperatureWindow, &buttonCloseTemperatureWindow, BTN_ID_6, 290, 0, 311, 20);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_6, BTN_STYLE_2D);

	UG_TextboxCreate(&temperatureWindow, &textBoxStepValueLabel, TXB_ID_3, 181, 142, 242, 158);
	UG_TextboxSetAlignment(&temperatureWindow, TXB_ID_3, ALIGN_CENTER_LEFT);
	temperatureGraphStepValue = 1;
	calculateGraphStepValueString(temperatureGraphStepValue);
	UG_TextboxSetText(&temperatureWindow, TXB_ID_3, WidgetsStrings.labelGraphStep);
	UG_TextboxSetFont(&temperatureWindow, TXB_ID_3, &FONT_6X8);

	UG_ButtonCreate(&temperatureWindow, &buttonDecrementGraphStep, BTN_ID_7, 243, 142, 259, 158);
	UG_ButtonSetText(&temperatureWindow, BTN_ID_7, "-");
	UG_ButtonSetFont(&temperatureWindow, BTN_ID_7, &FONT_6X8);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_7, BTN_STYLE_2D);

	UG_ButtonCreate(&temperatureWindow, &buttonIncrementGraphStep, BTN_ID_8, 262, 142, 278, 158);
	UG_ButtonSetText(&temperatureWindow, BTN_ID_8, "+");
	UG_ButtonSetFont(&temperatureWindow, BTN_ID_8, &FONT_6X8);
	UG_ButtonSetStyle(&temperatureWindow, BTN_ID_8, BTN_STYLE_2D);

	/**********************************
	* Create the setting window
	***********************************/
	UG_WindowCreate(&settingsWindow, settingsWindowObjects, MAX_OBJECTS_SETTINGS, settingsWindowHandler);
	UG_WindowSetTitleText(&settingsWindow, "options");
	UG_WindowSetTitleTextFont(&settingsWindow, &FONT_8X8);

	UG_TextboxCreate(&settingsWindow, &textBoxBrightnessValueLabel, TXB_ID_0, 1, FIRST_LINE_BEGIN_SW,
		100, FIRST_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_TextboxSetAlignment(&settingsWindow, TXB_ID_0, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&settingsWindow, TXB_ID_0, "brightness:");
	UG_TextboxSetFont(&settingsWindow, TXB_ID_0, &FONT_8X12);

	UG_TextboxCreate(&settingsWindow, &textBoxBrightnessValuePercents, TXB_ID_1, 101, FIRST_LINE_BEGIN_SW,
		139, FIRST_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_TextboxSetAlignment(&settingsWindow, TXB_ID_1, ALIGN_CENTER_LEFT);
	itoa(ClockState.brightness, WidgetsStrings.labelBrightnessValue, 10);
	UG_TextboxSetText(&settingsWindow, TXB_ID_1, WidgetsStrings.labelBrightnessValue);
	UG_TextboxSetFont(&settingsWindow, TXB_ID_1, &FONT_8X12);

	UG_TextboxCreate(&settingsWindow, &textBoxUpTime, TXB_ID_2, 1, SECOND_LINE_BEGIN_SW,
		300, SECOND_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_TextboxSetAlignment(&settingsWindow, TXB_ID_2, ALIGN_CENTER_LEFT);
	calculateUpTimeString();
	UG_TextboxSetText(&settingsWindow, TXB_ID_2, WidgetsStrings.labelUpTime);
	UG_TextboxSetFont(&settingsWindow, TXB_ID_2, &FONT_8X12);

	UG_TextboxCreate(&settingsWindow, &textBoxCalendar, TXB_ID_3, 1, CALENDAR_GRID_Y2_SW,
		125, CALENDAR_GRID_Y2_SW + 20);
	UG_TextboxSetAlignment(&settingsWindow, TXB_ID_3, ALIGN_CENTER_LEFT);
	calculateCalendarString(WidgetsStrings.labelCalendarValue, CalendarValueInSettings.day,
		CalendarValueInSettings.month, CalendarValueInSettings.year, " - ");
	UG_TextboxSetText(&settingsWindow, TXB_ID_3, WidgetsStrings.labelCalendarValue);
	UG_TextboxSetFont(&settingsWindow, TXB_ID_3, &FONT_8X12);

	UG_ButtonCreate(&settingsWindow, &buttonCloseOptionsWindow, BTN_ID_0, 290, 0, 311, 20);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_0, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonDecrementBrightness, BTN_ID_1, 140, FIRST_LINE_BEGIN_SW,
		140 + LINE_HEIGH_SW, FIRST_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_1, "-");
	UG_ButtonSetAlignment(&settingsWindow, BTN_ID_1, ALIGN_CENTER);
	UG_ButtonSetFont(&settingsWindow, BTN_ID_1, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_1, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonIncrementBrightness, BTN_ID_2, 165, FIRST_LINE_BEGIN_SW,
		165 + LINE_HEIGH_SW, FIRST_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_2, "+");
	UG_ButtonSetAlignment(&settingsWindow, BTN_ID_2, ALIGN_CENTER);
	UG_ButtonSetFont(&settingsWindow, BTN_ID_2, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_2, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonCalibrateLCD, BTN_ID_3, 1, THIRD_LINE_BEGIN_SW, 250,
		THIRD_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_3, "calibrate touch screen");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_3, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_3, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonFactoryReset, BTN_ID_4, 1, FOURTH_LINE_BEGIN_SW, 250,
		FOURTH_LINE_BEGIN_SW + LINE_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_4, "factory reset");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_4, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_4, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonSetCalendar, BTN_ID_5, 130, CALENDAR_GRID_Y2_SW, 250,
		CALENDAR_GRID_Y2_SW + 20);
	UG_ButtonSetText(&settingsWindow, BTN_ID_5, "change");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_5, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_5, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonIncrementDay, BTN_ID_6, CALENDAR_GRID_X1_SW, CALENDAR_GRID_Y1_SW,
		CALENDAR_GRID_X1_SW + CALENDAR_SWITCH_LENGTH_SW, CALENDAR_GRID_Y1_SW + CALENDAR_SWITCH_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_6, "");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_6, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_6, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonDecrementDay, BTN_ID_7, CALENDAR_GRID_X1_SW, CALENDAR_GRID_Y3_SW,
		CALENDAR_GRID_X1_SW + CALENDAR_SWITCH_LENGTH_SW, CALENDAR_GRID_Y3_SW + CALENDAR_SWITCH_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_7, "");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_7, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_7, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonIncrementMonth, BTN_ID_8, CALENDAR_GRID_X2_SW, CALENDAR_GRID_Y1_SW,
		CALENDAR_GRID_X2_SW + CALENDAR_SWITCH_LENGTH_SW, CALENDAR_GRID_Y1_SW + CALENDAR_SWITCH_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_8, "");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_8, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_8, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonDecrementMonth, BTN_ID_9, CALENDAR_GRID_X2_SW, CALENDAR_GRID_Y3_SW,
		CALENDAR_GRID_X2_SW + CALENDAR_SWITCH_LENGTH_SW, CALENDAR_GRID_Y3_SW + CALENDAR_SWITCH_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_9, "");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_9, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_9, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonIncrementYear, BTN_ID_10, CALENDAR_GRID_X3_SW, CALENDAR_GRID_Y1_SW,
		CALENDAR_GRID_X3_SW + CALENDAR_SWITCH_LENGTH_SW, CALENDAR_GRID_Y1_SW + CALENDAR_SWITCH_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_10, "");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_10, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_10, BTN_STYLE_2D);

	UG_ButtonCreate(&settingsWindow, &buttonDecrementYear, BTN_ID_11, CALENDAR_GRID_X3_SW, CALENDAR_GRID_Y3_SW,
		CALENDAR_GRID_X3_SW + CALENDAR_SWITCH_LENGTH_SW, CALENDAR_GRID_Y3_SW + CALENDAR_SWITCH_HEIGH_SW);
	UG_ButtonSetText(&settingsWindow, BTN_ID_11, "");
	UG_ButtonSetFont(&settingsWindow, BTN_ID_11, &FONT_8X12);
	UG_ButtonSetStyle(&settingsWindow, BTN_ID_11, BTN_STYLE_2D);

	setCalendarModification(false);

	/**********************************
	* Create the WiFi settings window
	***********************************/
	UG_WindowCreate(&wifiSettingsWindow, wifiSettingsObjects, MAX_OBJECTS_WIFI_SETTINGS, wifiSettingsWindowHandler);
	UG_WindowSetTitleText(&wifiSettingsWindow, "WiFi settings");
	UG_WindowSetTitleTextFont(&wifiSettingsWindow, &FONT_8X8);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxWifiStatus, TXB_ID_0, 1, 1, 280, 20);
	UG_TextboxSetAlignment(&wifiSettingsWindow, TXB_ID_0, ALIGN_CENTER_LEFT);
	strcpy(WidgetsStrings.labelWifiStatus, "Inactive");
	UG_TextboxSetText(&wifiSettingsWindow, TXB_ID_0, WidgetsStrings.labelWifiStatus);
	UG_TextboxSetFont(&wifiSettingsWindow, TXB_ID_0, &FONT_8X12);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxAssignedIpAddress, TXB_ID_1, 120, 25, 310, 45);
	UG_TextboxSetAlignment(&wifiSettingsWindow, TXB_ID_1, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&wifiSettingsWindow, TXB_ID_1, " ");
	UG_TextboxSetFont(&wifiSettingsWindow, TXB_ID_1, &FONT_8X12);
	UG_TextboxHide(&wifiSettingsWindow, TXB_ID_1);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxApnListLabel, TXB_ID_2, 1, 50, 300, 70);
	UG_TextboxSetAlignment(&wifiSettingsWindow, TXB_ID_2, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&wifiSettingsWindow, TXB_ID_2, "Access points:");
	UG_TextboxSetFont(&wifiSettingsWindow, TXB_ID_2, &FONT_8X12);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxNetwork[0], TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN, APN_NAME_BEGIN,
		FIRST_LINE_OF_APN_BEGIN_WSW, APN_NAME_END, FIRST_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxNetwork[1], (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + 1), APN_NAME_BEGIN,
		SECOND_LINE_OF_APN_BEGIN_WSW, APN_NAME_END, SECOND_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxNetwork[2], (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + 2), APN_NAME_BEGIN,
		THIRD_LINE_OF_APN_BEGIN_WSW, APN_NAME_END, THIRD_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxNetwork[3], (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + 3), APN_NAME_BEGIN,
		FOURTH_LINE_OF_APN_BEGIN_WSW, APN_NAME_END, FOURTH_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_TextboxCreate(&wifiSettingsWindow, &textBoxNetwork[4], (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + 4), APN_NAME_BEGIN,
		FIVETH_LINE_OF_APN_BEGIN_WSW, APN_NAME_END, FIVETH_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	for (int i = 0; i < NUMBER_OF_WIFI_DISPLAY_NETWORKS; i++)
	{
		UG_TextboxSetAlignment(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i), ALIGN_CENTER_LEFT);
		UG_TextboxSetText(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i), " ");
		UG_TextboxSetFont(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i), &FONT_8X12);
		UG_TextboxHide(&wifiSettingsWindow, (TEXTBOX_WIFI_DISPLAY_NETWORK_BEGIN + i));
	}

	UG_ButtonCreate(&wifiSettingsWindow, &buttonCloseWifiSettingsWindow, BTN_ID_0, 290, 0, 311, 20);
	UG_ButtonSetText(&wifiSettingsWindow, BTN_ID_0, " ");
	UG_ButtonSetFont(&wifiSettingsWindow, BTN_ID_0, &FONT_12X16);
	UG_ButtonSetStyle(&wifiSettingsWindow, BTN_ID_0, BTN_STYLE_2D);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonControlConnection, BTN_ID_1, 1, 25, 110, 45);
	UG_ButtonSetText(&wifiSettingsWindow, BTN_ID_1, "disconnect");
	UG_ButtonSetFont(&wifiSettingsWindow, BTN_ID_1, &FONT_8X12);
	UG_ButtonSetStyle(&wifiSettingsWindow, BTN_ID_1, BTN_STYLE_2D);
	UG_ButtonHide(&wifiSettingsWindow, BTN_ID_1);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonNetwork[0], BUTTON_WIFI_DISPLAY_NETWORK_BEGIN, APN_CONNECT_BUTTON_BEGIN,
		FIRST_LINE_OF_APN_BEGIN_WSW, APN_CONNECT_BUTTON_END, FIRST_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonNetwork[1], (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + 1), APN_CONNECT_BUTTON_BEGIN,
		SECOND_LINE_OF_APN_BEGIN_WSW, APN_CONNECT_BUTTON_END, SECOND_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonNetwork[2], (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + 2), APN_CONNECT_BUTTON_BEGIN,
		THIRD_LINE_OF_APN_BEGIN_WSW, APN_CONNECT_BUTTON_END, THIRD_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonNetwork[3], (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + 3), APN_CONNECT_BUTTON_BEGIN,
		FOURTH_LINE_OF_APN_BEGIN_WSW, APN_CONNECT_BUTTON_END, FOURTH_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonNetwork[4], (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + 4), APN_CONNECT_BUTTON_BEGIN,
		FIVETH_LINE_OF_APN_BEGIN_WSW, APN_CONNECT_BUTTON_END, FIVETH_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);

	for (int i = 0; i < NUMBER_OF_WIFI_DISPLAY_NETWORKS; i++)
	{
		UG_ButtonSetText(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i), " ");
		UG_ButtonSetFont(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i), &FONT_8X12);
		UG_ButtonSetStyle(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i), BTN_STYLE_2D);
		UG_ButtonHide(&wifiSettingsWindow, (BUTTON_WIFI_DISPLAY_NETWORK_BEGIN + i));
	}

	UG_ButtonCreate(&wifiSettingsWindow, &buttonUpNetworkList, BTN_ID_8, APN_SLIDER_BEGIN,
		FIRST_LINE_OF_APN_BEGIN_WSW, APN_SLIDER_END, FIRST_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);
	UG_ButtonSetText(&wifiSettingsWindow, BTN_ID_8, " ");
	UG_ButtonSetFont(&wifiSettingsWindow, BTN_ID_8, &FONT_8X12);
	UG_ButtonSetStyle(&wifiSettingsWindow, BTN_ID_8, BTN_STYLE_2D);
	UG_ButtonHide(&wifiSettingsWindow, BTN_ID_8);

	UG_ButtonCreate(&wifiSettingsWindow, &buttonDownNetworkList, BTN_ID_9, APN_SLIDER_BEGIN,
		FIVETH_LINE_OF_APN_BEGIN_WSW, APN_SLIDER_END, FIVETH_LINE_OF_APN_BEGIN_WSW + APN_LIST_LINE_HEIGH_WSW);
	UG_ButtonSetText(&wifiSettingsWindow, BTN_ID_9, " ");
	UG_ButtonSetFont(&wifiSettingsWindow, BTN_ID_9, &FONT_8X12);
	UG_ButtonSetStyle(&wifiSettingsWindow, BTN_ID_9, BTN_STYLE_2D);
	UG_ButtonHide(&wifiSettingsWindow, BTN_ID_9);

	/**********************************
	* Create the WiFi Keyboard Window
	***********************************/
	UG_WindowCreate(&wifiKeyboardWindow, wifiKeyboardObjects, MAX_OBJECTS_KEYBOARD_WINDOW, wifiKeyboardWindowHandler);
	UG_WindowSetTitleText(&wifiKeyboardWindow, "WiFi password");
	UG_WindowSetTitleTextFont(&wifiKeyboardWindow, &FONT_8X8);

	UG_TextboxCreate(&wifiKeyboardWindow, &textBoxApnLabel, TXB_ID_0, 1, 3, 40, 18);
	UG_TextboxSetAlignment(&wifiKeyboardWindow, TXB_ID_0, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_0, "APN:");
	UG_TextboxSetFont(&wifiKeyboardWindow, TXB_ID_0, &FONT_8X12);

	UG_TextboxCreate(&wifiKeyboardWindow, &textBoxChosenApnLabel, TXB_ID_1, 41, 3, 280, 18);
	UG_TextboxSetAlignment(&wifiKeyboardWindow, TXB_ID_1, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_1, " ");
	UG_TextboxSetFont(&wifiKeyboardWindow, TXB_ID_1, &FONT_8X12);

	UG_TextboxCreate(&wifiKeyboardWindow, &textBoxPApnPasswordLabel, TXB_ID_2, 1, 20, 280, 35);
	UG_TextboxSetAlignment(&wifiKeyboardWindow, TXB_ID_2, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_2, "Password: ");
	UG_TextboxSetFont(&wifiKeyboardWindow, TXB_ID_2, &FONT_8X12);

	UG_TextboxCreate(&wifiKeyboardWindow, &textBoxPApnPasswordContent, TXB_ID_3, 75, 20, 290, 35);
	UG_TextboxSetAlignment(&wifiKeyboardWindow, TXB_ID_3, ALIGN_CENTER_LEFT);
	UG_TextboxSetText(&wifiKeyboardWindow, TXB_ID_3, WidgetsStrings.labelApnPassword);
	UG_TextboxSetFont(&wifiKeyboardWindow, TXB_ID_3, &FONT_8X12);

	UG_CheckboxCreate(&wifiKeyboardWindow, &ckeckBoxShowApnPassword, CHB_ID_0, 1, 40, 290, 60);
	UG_CheckboxSetText(&wifiKeyboardWindow, CHB_ID_0, "show password");
	UG_CheckboxSetFont(&wifiKeyboardWindow, CHB_ID_0, &FONT_6X8);
	UG_CheckboxSetCheched(&wifiKeyboardWindow, CHB_ID_0, CHB_STATE_RELEASED);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonCloseKeyboardWindow, BTN_ID_0, 290, 0, 311, 20);
	UG_ButtonSetText(&wifiKeyboardWindow, BTN_ID_0, " ");
	UG_ButtonSetFont(&wifiKeyboardWindow, BTN_ID_0, &FONT_12X16);
	UG_ButtonSetStyle(&wifiKeyboardWindow, BTN_ID_0, BTN_STYLE_2D);

	//Keyboard key begin
	//first row
	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyQor1Num1a, BUTTON_WIFI_KEYBOARD_BEGIN, BUTTON_COLUMN_BEGIN_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_BEGIN_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyWor2Num2a, (BUTTON_WIFI_KEYBOARD_BEGIN + 1), BUTTON_COLUMN_NR2_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR2_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyEor3Num3a, (BUTTON_WIFI_KEYBOARD_BEGIN + 2), BUTTON_COLUMN_NR4_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR4_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyRor4Num4a, (BUTTON_WIFI_KEYBOARD_BEGIN + 3), BUTTON_COLUMN_NR6_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR6_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyTor5Num5a, (BUTTON_WIFI_KEYBOARD_BEGIN + 4), BUTTON_COLUMN_NR8_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR8_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyYor6Num6a, (BUTTON_WIFI_KEYBOARD_BEGIN + 5), BUTTON_COLUMN_NR10_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR10_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyUor7Num7a, (BUTTON_WIFI_KEYBOARD_BEGIN + 6), BUTTON_COLUMN_NR12_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR12_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyIor8Num8a, (BUTTON_WIFI_KEYBOARD_BEGIN + 7), BUTTON_COLUMN_NR14_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR14_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyOor9Num9a, (BUTTON_WIFI_KEYBOARD_BEGIN + 8), BUTTON_COLUMN_NR16_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR16_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyPor0Num10a, (BUTTON_WIFI_KEYBOARD_BEGIN + 9), BUTTON_COLUMN_NR18_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR18_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyGraveAccentOrTildeNum11a, (BUTTON_WIFI_KEYBOARD_BEGIN + 10), BUTTON_COLUMN_NR20_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR20_WKW + BUTTON_LENGTH_WKW,
		FIRST_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	//Second row
	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyAorMonkeyNum1b, (BUTTON_WIFI_KEYBOARD_BEGIN + 11), BUTTON_COLUMN_NR1_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR1_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeySorHashNum2b, (BUTTON_WIFI_KEYBOARD_BEGIN + 12), BUTTON_COLUMN_NR3_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR3_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyDorDolarNum3b, (BUTTON_WIFI_KEYBOARD_BEGIN + 13), BUTTON_COLUMN_NR5_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR5_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyForFloorNum4b, (BUTTON_WIFI_KEYBOARD_BEGIN + 14), BUTTON_COLUMN_NR7_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR7_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyGorAmpersandNum5b, (BUTTON_WIFI_KEYBOARD_BEGIN + 15), BUTTON_COLUMN_NR9_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR9_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyHorMinusNum6b, (BUTTON_WIFI_KEYBOARD_BEGIN + 16), BUTTON_COLUMN_NR11_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR11_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyJorPlusNum7b, (BUTTON_WIFI_KEYBOARD_BEGIN + 17), BUTTON_COLUMN_NR13_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR13_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyKorAsteriskNum8b, (BUTTON_WIFI_KEYBOARD_BEGIN + 18), BUTTON_COLUMN_NR15_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR15_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyLorExclamationMarkNum9b, (BUTTON_WIFI_KEYBOARD_BEGIN + 19),
		BUTTON_COLUMN_NR17_WKW, SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR17_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyRightBracesOrRightSquareBracketsNum10b, (BUTTON_WIFI_KEYBOARD_BEGIN + 20),
		BUTTON_COLUMN_NR19_WKW, SECOND_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR19_WKW + BUTTON_LENGTH_WKW,
		SECOND_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	//Third row
	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyShiftNum1c, (BUTTON_WIFI_KEYBOARD_BEGIN + SHIFT_KEY_OFFSET_WKW), BUTTON_COLUMN_BEGIN_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_BEGIN_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);
	UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + SHIFT_KEY_OFFSET_WKW), "\x18");
	UG_ButtonSetFont(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + SHIFT_KEY_OFFSET_WKW), &FONT_8X12);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyZorQuotationMarksSingleNum2c, (BUTTON_WIFI_KEYBOARD_BEGIN + 22),
		BUTTON_COLUMN_NR2_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR2_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyXorQuotationMarksDoubleNum3c, (BUTTON_WIFI_KEYBOARD_BEGIN + 23),
		BUTTON_COLUMN_NR4_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR4_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyCorQuestionMarkNum4c, (BUTTON_WIFI_KEYBOARD_BEGIN + 24),
		BUTTON_COLUMN_NR6_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR6_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyVorEqualsSignNum5c, (BUTTON_WIFI_KEYBOARD_BEGIN + 25),
		BUTTON_COLUMN_NR8_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR8_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyBorBackslashNum6c, (BUTTON_WIFI_KEYBOARD_BEGIN + 26),
		BUTTON_COLUMN_NR10_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR10_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyNorPercent1Num7c, (BUTTON_WIFI_KEYBOARD_BEGIN + 27),
		BUTTON_COLUMN_NR12_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR12_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyMorForwardSlashNum8c, (BUTTON_WIFI_KEYBOARD_BEGIN + 28),
		BUTTON_COLUMN_NR14_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR14_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyLeftBracesOrLeftSquareBracketsNum9c, (BUTTON_WIFI_KEYBOARD_BEGIN + 29),
		BUTTON_COLUMN_NR16_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR16_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyCaretOrVerticalBarNum10c, (BUTTON_WIFI_KEYBOARD_BEGIN + 30),
		BUTTON_COLUMN_NR18_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR18_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyBackspaceNum11c, (BUTTON_WIFI_KEYBOARD_BEGIN + BACKSPACE_KEY_OFFSET_WKW),
		BUTTON_COLUMN_NR20_WKW, THIRD_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR20_WKW + BUTTON_LENGTH_WKW,
		THIRD_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);
	UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + BACKSPACE_KEY_OFFSET_WKW), "\x1b");
	UG_ButtonSetFont(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + BACKSPACE_KEY_OFFSET_WKW), &FONT_8X12);

	//Fourth row
	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeySwitchToSignNum1d,
		(BUTTON_WIFI_KEYBOARD_BEGIN + LETTER_OR_SPECIAL_CHAR_KEY_OFFSET_WKW),
		BUTTON_COLUMN_BEGIN_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR3_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);
	UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + LETTER_OR_SPECIAL_CHAR_KEY_OFFSET_WKW), "QWE/12?");
	UG_ButtonSetFont(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + LETTER_OR_SPECIAL_CHAR_KEY_OFFSET_WKW), &FONT_6X8);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyDotOrColonNum2d, (BUTTON_WIFI_KEYBOARD_BEGIN + 33),
		BUTTON_COLUMN_NR5_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR5_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyCommaQorSemicolonNum3d, (BUTTON_WIFI_KEYBOARD_BEGIN + 34),
		BUTTON_COLUMN_NR7_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR7_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeySpaceNum4d, (BUTTON_WIFI_KEYBOARD_BEGIN + 35),
		BUTTON_COLUMN_NR9_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR11_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyLeftRoundBracketOrLeftChevronNum5d, (BUTTON_WIFI_KEYBOARD_BEGIN + 36),
		BUTTON_COLUMN_NR13_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR13_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyRightRoundBracketOrRightChevronNum6d, (BUTTON_WIFI_KEYBOARD_BEGIN + 37),
		BUTTON_COLUMN_NR15_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR15_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);

	UG_ButtonCreate(&wifiKeyboardWindow, &buttonKeyEnterNum7d, (BUTTON_WIFI_KEYBOARD_BEGIN + ENTER_KEY_OFFSET_WKW),
		BUTTON_COLUMN_NR17_WKW, FOURTH_ROW_OF_BUTTONS_BEGIN_WKW, BUTTON_COLUMN_NR20_WKW + BUTTON_LENGTH_WKW,
		FOURTH_ROW_OF_BUTTONS_BEGIN_WKW + BUTTON_HEIGH_WKW);
	UG_ButtonSetText(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + ENTER_KEY_OFFSET_WKW), "Enter");
	UG_ButtonSetFont(&wifiKeyboardWindow, (BUTTON_WIFI_KEYBOARD_BEGIN + ENTER_KEY_OFFSET_WKW), &FONT_6X8);

	setKeyboardButtons(KeyboardSignsGlobalState);

	UG_WindowShow(&mainWindow);
	//UG_WindowShow(&clockSettingsWindow);
	//UG_WindowShow(&temperatureWindow);
	//UG_WindowShow(&settingsWindow);
	//UG_WindowShow(&wifiSettingsWindow);
	//UG_WindowShow(&wifiKeyboardWindow);

	UG_Update();
}
