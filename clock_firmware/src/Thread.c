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

#include "Thread.h"

static ClockStateType ClockStateFramBuffer;
static TemperatureSingleDayRecordType TemperatureSingleDayRecordBuffer;
static uint8_t clearFramData[CLEAR_BLOCK_SIZE];
static const uint8_t SoundAlarmTable[LENGHT_OF_SOUND_ALARM_TABLE] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1};
static WifiStateType WifiStateStructure;

/*****************************************************************************************
* convertFramIndexToAddress() - calculate FRAM memory address using index of block with
* temperature per day structure.
*
* Parameters:
* @index: value in range from 0 to MAX_RECORD_IN_FRAM with number of selected
*  TemperatureSingleDayRecordType structure in FRAM.
*
* Return: FRAM memory address where index of TemperatureSingleDayRecordType structure
* is located.
*****************************************************************************************/
static uint16_t convertFramIndexToAddress(uint16_t index)
{
	return (FRAM_MEASUREMENT_DATA_BEGIN + (sizeof(TemperatureSingleDayRecordType)*index));
}

/*****************************************************************************************
* nextDayTemperatureStructureInit() - increase date about one day in TemperatureSingleDayRecordType
* structure located in memory address hold by pointerToStructure argument. Function also
* initialize all temperature values in TemperatureSingleDayRecordType structure as
* INVALID_READ_SENSOR_VALUE.
*
* Parameters:
* @pointerToStructure: pointer to TemperatureSingleDayRecordType which will be modified.
*
*****************************************************************************************/
static void nextDayTemperatureStructureInit(TemperatureSingleDayRecordType *pointerToStructure)
{
	//use values from previous block to calculate new block
	GUI_IncrementDay(&pointerToStructure->day, &pointerToStructure->month, &pointerToStructure->year);

	//fill temperatures record using invalid temperature value
	for(uint16_t i = 0; i < MAX_TEMP_RECORD_PER_DAY; i++)
	{
		pointerToStructure->temperatureValues[i] = INVALID_READ_SENSOR_VALUE;
	}
}

/*****************************************************************************************
* verifyTemperatureRecord() - verify that TemperatureSingleDayRecordType structure under
* memory hold by temperatureSingleDay pointer is correct. Correctness mean that values of
* fields day, month, year and source of temperature sensor is the same like parameters of
* this function. This function also check CRC field to confirm that data ins structure is
* valid.
*
* Parameters:
* @temperatureSingleDay: pointer to TemperatureSingleDayRecordType which will be verified.
* @sourceTemperature: type of temperature sensor used to comarison with tested structure.
* @day: day number used to comarison with tested structure.
* @month: month number used to comarison with tested structure.
* @year: year number used to comarison with tested structure.
*
* Return: true if tested structure is valid otherwise false.
*****************************************************************************************/
bool verifyTemperatureRecord(TemperatureSingleDayRecordType *temperatureSingleDay,
		uint8_t sourceTemperature, uint8_t day, uint8_t month, uint8_t year)
{
	//calculate and compare checksums
	uint16_t checkSumValue = Chip_CRC_CRC16((uint16_t*)temperatureSingleDay, (offsetof(TemperatureSingleDayRecordType, CRC16Value)/2));

	if(checkSumValue != temperatureSingleDay->CRC16Value)
		return false;

	if(temperatureSingleDay->day == day
		&& temperatureSingleDay->month == month
		&& temperatureSingleDay->year == year
		&& temperatureSingleDay->source == sourceTemperature)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*****************************************************************************************
* temperatureRecordLoader() - load temperature structure from address set as paramatere to
* memory address hold by temperatureSingleDay pointer. Loaded TemperatureSingleDayRecordType
* structure fields will be checked that fields are correct. Timestamp in structure will be
* compared with current timestamp hold by ClockState structure. Function also check that
* tested structure not contain only zeroes as fields values. This function is blocking
* and is call on startup procedure.
*
* Parameters:
* @address: FRAM memory address where TemperatureSingleDayRecordType structure is stored.
* @temperatureSingleDay: pointer to RAM memory where TemperatureSingleDayRecordType
*  structure will be loaded.
* @sourceTemperature: type of temperature sensor used to comarison with tested structure.
*
* Return: true if tested structure is valid otherwise false.
*****************************************************************************************/
static bool temperatureRecordLoader(uint16_t address, TemperatureSingleDayRecordType *temperatureSingleDay,
		uint8_t sourceTemperature)
{
	FRAM_Read(address, sizeof(TemperatureSingleDayRecordType), (uint8_t*)temperatureSingleDay);

	for(int i = 0; i<100000; i++)
	{
		if(FRAM_Process())
		{
			i = 100000;
		}
		ClockSleep(1);
	}

	//if temperatureSingleDay contain only zeros treat as invalid
	bool emptyStructure = true;
	for(uint16_t i = 0; i < (uint16_t)(sizeof(TemperatureSingleDayRecordType)); i++)
	{
		uint8_t* temperatureDataPointer = (uint8_t*)(temperatureSingleDay);
		if(temperatureDataPointer[i] != 0)
		{
			emptyStructure = false;
			break;
		}
	}

	if(emptyStructure == true)
		return false;

	//validate structure content - checksum, timestamp, temperature source
	return verifyTemperatureRecord(temperatureSingleDay, sourceTemperature, ClockState.day, ClockState.month, ClockState.year);
}

/*****************************************************************************************
* initTemperatureBlock() - load last temperature day structure from FRAM memory and
* verified correctness. FRAM index before using will be checked that is corrected.
* If data pointed by FRAM index will be incorrect that in second atempt is loaded and
* tested data from FRAM_MEASUREMENT_BACKUP_COPY address. Function call is performed on
* beginning of startup process and procedure is blocking which not cause problem beacuse
* call is performed just once for all sensors. This function is directly call from
* Thread_Init function and call chain of above functions.
*
* Parameters:
* @temperatureSingleDay: pointer to RAM memory where TemperatureSingleDayRecordType
*  structure will be loaded.
* @temperatureFramIndex: value in range from 0 to MAX_RECORD_IN_FRAM with number of
*  selected TemperatureSingleDayRecordType structure in FRAM.
* @sourceTemperature: type of temperature sensor used to comarison with tested structure.
*
*****************************************************************************************/
static void initTemperatureBlock(TemperatureSingleDayRecordType *temperatureSingleDay, uint16_t *temperatureFramIndex, uint8_t sourceTemperature)
{
	//FRAM index should be initialized
	if(*temperatureFramIndex != NOT_INITIALIZED_FRAM_INDEX_VALUE)
	{
		//load data from FRAM and validate structure
		if(temperatureRecordLoader(convertFramIndexToAddress(*temperatureFramIndex), temperatureSingleDay, sourceTemperature))
		{

		}
		//if previous data was wrong then check backup FRAM sector
		else if(temperatureRecordLoader(FRAM_MEASUREMENT_BACKUP_COPY, temperatureSingleDay, sourceTemperature))
		{

		}
		else
		{
			//if in two source data is incorrect then reinit structure
			GUI_InitTemperatureStructure(sourceTemperature, ClockState.day,
				ClockState.month, ClockState.year, temperatureSingleDay);
		}
	}
	else
	{
		GUI_InitTemperatureStructure(sourceTemperature, ClockState.day,
			ClockState.month, ClockState.year, temperatureSingleDay);

		*temperatureFramIndex = GUI_ReturnNewFramIndex();
	}
}

/*****************************************************************************************
* lockSharedSpiPort() - check that SPI port shared between touchscreen and FRAM can be
* used by FRAM transaction. FRAM is locked to perform appropriate operation when touch
* screen isn't pressed(pin state which inform about touch state is checked) and other
* FRAM transaction isn't processed.
*
* Parameters:
* @selectedSpiType: type of SPI transaction which will be used as lock flag if lock
*  operation will be possible.
*
* Return: true if lock operation can be possible othervise return false.
*****************************************************************************************/
static bool lockSharedSpiPort(SHARED_SPI_STATE_TYPE selectedSpiType)
{
	//check SPI availability and PENIRQ on lcd is on high state(screen wasn't pressed)
	if( (ClockState.sharedSpiState == NOT_USED)
		&& (GPIO_GetState(TOUCH_PANEL_PIN_PENIRQ_GPIO_PORT, TOUCH_PANEL_PIN_PENIRQ_GPIO_PIN) == true))
	{
		//lock SPI
		ClockState.sharedSpiState = selectedSpiType;

		return true;
	}
	else
	{
		return false;
	}
}

/*****************************************************************************************
* processWriteTemperature() - copied data from day buffer located in RAM memory to FRAM
* memory. Write to FRAM memory is prformed only if recordTemperature flag is set to
* true. Multiply temperature source can have this flag set as true and parallel
* temperature store in FRAM will be performed(when first store will be finished then next
* temperature store process will be performed). This function is non blocking and mus be
* call cyclically.
*
* Parameters:
* @temperatureFramTransaction: pointer to structure with variable necessary for perform
*  copy to FRAM process. All temperature sensor must have separate structure. This
*  structure hold few temperature measurements which will be used to calculate average
*  value.
*
*****************************************************************************************/
static void processWriteTemperature(TemperatureFramWriteTransactionPackageType *temperatureFramTransaction)
{
	//check that record temperature is ON
	if(ClockState.TemperatureSensorTable[temperatureFramTransaction->source].recordTemperature == true)
	{
		//store temperature in table
		if((((ClockState.currentTimeMinute + (uint8_t)1)%3) == 0)
			&& (ClockState.currentTimeMinute != temperatureFramTransaction->previousAssignFilter))
		{
			temperatureFramTransaction->previousAssignFilter = ClockState.currentTimeMinute;

			//this if statememnt protect about exceed memory which can happen when user dynamic change current clock minute
			if(temperatureFramTransaction->valuesCounter < NUMBER_OF_MEASUREMENTS_IN_FILTER_TABLE)
			{
				temperatureFramTransaction->temperatureTableFilter[temperatureFramTransaction->valuesCounter]
				    = ClockState.TemperatureSensorTable[temperatureFramTransaction->source].temperatureValue;
				temperatureFramTransaction->valuesCounter++;
			}
		}

		//update structure and send request to start FRAM store process
		if((((ClockState.currentTimeMinute + (uint8_t)1)%15) == 0)
			&& (ClockState.currentTimeMinute != temperatureFramTransaction->previousAssignStore))
		{
			temperatureFramTransaction->previousAssignStore = ClockState.currentTimeMinute;

			uint16_t filteredValueTmp = 0;
			uint8_t correctValuesCounter = 0;

			for(uint8_t i = 0; i < temperatureFramTransaction->valuesCounter;i++)
			{
				if(temperatureFramTransaction->temperatureTableFilter[i] != INVALID_READ_SENSOR_VALUE)
				{
					filteredValueTmp += temperatureFramTransaction->temperatureTableFilter[i];
					correctValuesCounter++;
				}
			}

			if(correctValuesCounter > 0)
			{
				filteredValueTmp /= correctValuesCounter;
			}
			else
			{
				filteredValueTmp = INVALID_READ_SENSOR_VALUE;
			}

			temperatureFramTransaction->valuesCounter = 0;

			//recalculate index
			temperatureFramTransaction->temperatureIndex = (((ClockState.currentTimeHour*60) + ClockState.currentTimeMinute + 1) / 15) - 1;
			TemperatureSingleDay[temperatureFramTransaction->source].temperatureValues[temperatureFramTransaction->temperatureIndex] = filteredValueTmp;
			temperatureFramTransaction->startTemperatureTransaction = true;
		}
	}/* if(ClockState.TemperatureSensorTable[temperatureFramTransaction->source].recordTemperature == true) */

	if(temperatureFramTransaction->startTemperatureTransaction && lockSharedSpiPort(FRAM_USAGE))
	{
		ClockState.FramTransactionIdentifier = temperatureFramTransaction->transactionId;

		//copy to local buffer and calculate CRC
		TemperatureSingleDayRecordBuffer = TemperatureSingleDay[temperatureFramTransaction->source];
		TemperatureSingleDayRecordBuffer.CRC16Value = Chip_CRC_CRC16((uint16_t*)&TemperatureSingleDayRecordBuffer, (offsetof(TemperatureSingleDayRecordType, CRC16Value)/2));

		//send FRAM write request to copy all day measurement to backup area
		FRAM_Write(FRAM_MEASUREMENT_BACKUP_COPY, sizeof(TemperatureSingleDayRecordType), (uint8_t*)&TemperatureSingleDayRecordBuffer);
		temperatureFramTransaction->transactionStep = TRANSACTION_COPY_TO_BACKUP;
	}

	//when copy to backup area will be finished then start copy to dedicated area
	if((ClockState.FramTransactionIdentifier == temperatureFramTransaction->transactionId)
		&& (temperatureFramTransaction->transactionStep == TRANSACTION_COPY_TO_BACKUP) && FRAM_Process())
	{
		FRAM_Write(FRAM_MEASUREMENT_DATA_BEGIN + (sizeof(TemperatureSingleDayRecordType)*(ClockState.TemperatureSensorTable[temperatureFramTransaction->source].temperatureFramIndex)),
			sizeof(TemperatureSingleDayRecordType), (uint8_t*)&TemperatureSingleDayRecordBuffer);
		temperatureFramTransaction->transactionStep = TRANSACTION_COPY_TO_DEDICATED_AREA;
	}

	//when copy to dedicated area will be finished unlock SPI port
	if((ClockState.FramTransactionIdentifier == temperatureFramTransaction->transactionId)
		&& (temperatureFramTransaction->transactionStep == TRANSACTION_COPY_TO_DEDICATED_AREA) && FRAM_Process())
	{
		ClockState.sharedSpiState = NOT_USED;
		ClockState.FramTransactionIdentifier = FRAM_ID_NOP;
		temperatureFramTransaction->transactionStep = TRANSACTION_NOT_DEFINED;
		temperatureFramTransaction->startTemperatureTransaction = false;

		//if last element of table was assigned then create new table with values for new day
		if((ClockState.TemperatureSensorTable[temperatureFramTransaction->source].recordTemperature == true)
			&& (temperatureFramTransaction->temperatureIndex == (MAX_TEMP_RECORD_PER_DAY - 1)))
		{
			nextDayTemperatureStructureInit(&TemperatureSingleDay[temperatureFramTransaction->source]);
			ClockState.TemperatureSensorTable[temperatureFramTransaction->source].temperatureFramIndex = GUI_ReturnNewFramIndex();
		}
	}
}

/*****************************************************************************************
* processReadTemperature() - copied data from FRAN memory to RAM buffer. Function is used
* to gather data from FRAM which will be used to draw graph. Function in one cycle operate
* on one sensor data source. TemperatureFramReadTransactionPackageType structure used as
* input parameter is initialized when window with temperature visualization
* (temperatureWindow) is opended. Global BufferCursor structure is trigered from GUI level
* and contain cotrol data for this function. RAM buffer used to load temperature structures
* contain previous buffer, next buffer and current pointed structure. All RAM buffer
* objects is organized as table with ReadFramTempBufferType structures where appropriate
* pointers hold address of next and previous element of table. Overall size of table
* describe define READ_TEMP_FRAM_BUFFER_SIZE which is calculated from two defines with size
* of previous buffer and next buffer. Those defines are READ_TEMP_FRAM_BUFFER_PREVIOUS and
* READ_TEMP_FRAM_BUFFER_NEXT. This function is non blocking and mus be call cyclically.
*
* Parameters:
* @temperatureFramTransaction: pointer to structure with data which decide about load
*  process. Structure under pointer memory contain temporary data like day measueremnt
*  structure and FRAM index which will be assigned to RAM buffer object if searched data
*  will match.
*
*****************************************************************************************/
static void processReadTemperature(TemperatureFramReadTransactionPackageType *temperatureFramTransaction)
{
	//check flag which is set in GUI thread when temperature widow is opened and other precondition is fulfiled
	if(BufferCursor.loadDataFlag == true || temperatureFramTransaction->startReadTransaction == true)
	{
		if(temperatureFramTransaction->startReadTransaction == false)
		{
			temperatureFramTransaction->pointerToStructureTmp = BufferCursor.structPointer;

			//check that data in previous structure from cursor is available
			for(int i = 0; i < READ_TEMP_FRAM_BUFFER_PREVIOUS; i++)
			{
				uint16_t framIndexFromCurrentStructure = temperatureFramTransaction->pointerToStructureTmp->framIndex;
				uint16_t framIndexFromPreviousStructure = ((ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp->pointerToPreviousElement)->framIndex;

				//get day value of time stamp in current structure
				temperatureFramTransaction->dayTmp = temperatureFramTransaction->pointerToStructureTmp->singleRecord.day;
				temperatureFramTransaction->monthTmp = temperatureFramTransaction->pointerToStructureTmp->singleRecord.month;
				temperatureFramTransaction->yearTmp = temperatureFramTransaction->pointerToStructureTmp->singleRecord.year;

				temperatureFramTransaction->pointerToStructureTmp = (ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp->pointerToPreviousElement;

				if( ((ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp)->notExistFlag == true)
				{
					break;
				}

				//test that FRAM index value in previous structure is correct(pass rule mean that it isn't correct)
				if( ((ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp->availabilityFlag == false)
					|| (framIndexFromCurrentStructure < framIndexFromPreviousStructure)
					|| ( (framIndexFromCurrentStructure > (MAX_RECORD_IN_FRAM - SEARCH_RADIUS))
						&& (framIndexFromPreviousStructure < SEARCH_RADIUS) ) )
				{
					//exception of rule - correct occurence
					if( (((ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp)->availabilityFlag == true)
						&& (framIndexFromPreviousStructure > (MAX_RECORD_IN_FRAM - SEARCH_RADIUS))
						&& (framIndexFromCurrentStructure < SEARCH_RADIUS) )
					{

					}
					else
					{
						//get position where search of next day will be started
						temperatureFramTransaction->searchFramIndexPosition = GUI_GetDecrementedFramIndex(framIndexFromCurrentStructure);

						//calculate value which will be used during search
						GUI_DecrementDay(&temperatureFramTransaction->dayTmp, &temperatureFramTransaction->monthTmp, &temperatureFramTransaction->yearTmp);

						temperatureFramTransaction->startReadTransaction = true;
						temperatureFramTransaction->moveBackward = true;
						temperatureFramTransaction->searchCounter = 0;

						goto endCheck;
					}
				}
			}/* for(int i = 0; i < READ_TEMP_FRAM_BUFFER_PREVIOUS; i++) */

			//verify that current max index isn't equal as present day
			if(BufferCursor.structPointer->framIndex != ClockState.TemperatureSensorTable[BufferCursor.source].temperatureFramIndex)
			{
				temperatureFramTransaction->pointerToStructureTmp = BufferCursor.structPointer;

				//check that data in previous structure from cursor is available
				for(int i = 0; i < READ_TEMP_FRAM_BUFFER_NEXT; i++)
				{
					uint16_t framIndexFromCurrentStructure = temperatureFramTransaction->pointerToStructureTmp->framIndex;
					uint16_t framIndexFromNextStructure = ((ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp->pointerToNextElement)->framIndex;

					//get day value of time stamp in current structure
					temperatureFramTransaction->dayTmp = temperatureFramTransaction->pointerToStructureTmp->singleRecord.day;
					temperatureFramTransaction->monthTmp = temperatureFramTransaction->pointerToStructureTmp->singleRecord.month;
					temperatureFramTransaction->yearTmp = temperatureFramTransaction->pointerToStructureTmp->singleRecord.year;

					temperatureFramTransaction->pointerToStructureTmp = (ReadFramTempBufferType*)temperatureFramTransaction->pointerToStructureTmp->pointerToNextElement;

					//next structure is equal as present day structure value
					if(framIndexFromNextStructure == ClockState.TemperatureSensorTable[BufferCursor.source].temperatureFramIndex)
					{
						goto endCheck;
					}

					//test that FRAM index value in next structure is correct(pass rule mean that it isn't correct)
					if( (framIndexFromCurrentStructure > framIndexFromNextStructure)
						|| ( (framIndexFromCurrentStructure < SEARCH_RADIUS)
							&& (framIndexFromNextStructure > (MAX_RECORD_IN_FRAM - SEARCH_RADIUS)) ) )
					{
						//exception of rule - correct occurence
						if((framIndexFromCurrentStructure > (MAX_RECORD_IN_FRAM - SEARCH_RADIUS))
							&& (framIndexFromNextStructure < SEARCH_RADIUS))
						{

						}
						else
						{
							//get position where search of next day will be started
							temperatureFramTransaction->searchFramIndexPosition = GUI_GetIncrementedFramIndex(framIndexFromCurrentStructure);

							//calculate value which will be used during search
							GUI_IncrementDay(&temperatureFramTransaction->dayTmp, &temperatureFramTransaction->monthTmp, &temperatureFramTransaction->yearTmp);

							temperatureFramTransaction->startReadTransaction = true;
							temperatureFramTransaction->moveBackward = false;
							temperatureFramTransaction->searchCounter = 0;

							goto endCheck;
						}
					}
				}/* for(int i = 0; i < READ_TEMP_FRAM_BUFFER_NEXT; i++) */
			}/* if(BufferCursor.structPointer->framIndex != ClockState.TemperatureSensorTable[BufferCursor.source].temperatureFramIndex) */
endCheck:
			;
		}
		else//if forward load is necessary
		{
			if(lockSharedSpiPort(FRAM_USAGE))
			{
				ClockState.FramTransactionIdentifier = FRAM_ID_READ_TEMPERATURE;

				//start searchin place pointed by index searchFramIndexPosition
				FRAM_Read(convertFramIndexToAddress(temperatureFramTransaction->searchFramIndexPosition),
					sizeof(TemperatureSingleDayRecordType), (uint8_t*)&temperatureFramTransaction->temperatureSingleDayTmp);
			}

			if((ClockState.sharedSpiState == FRAM_USAGE)
				&& (ClockState.FramTransactionIdentifier == FRAM_ID_READ_TEMPERATURE) && FRAM_Process())
			{
				//call verify function which return state
				if(verifyTemperatureRecord(&temperatureFramTransaction->temperatureSingleDayTmp, BufferCursor.source,
					temperatureFramTransaction->dayTmp, temperatureFramTransaction->monthTmp, temperatureFramTransaction->yearTmp))
				{
					//value is correct assign it to appropriate place
					temperatureFramTransaction->pointerToStructureTmp->availabilityFlag = true;
					temperatureFramTransaction->pointerToStructureTmp->notExistFlag = false;
					temperatureFramTransaction->pointerToStructureTmp->singleRecord = temperatureFramTransaction->temperatureSingleDayTmp;
					temperatureFramTransaction->pointerToStructureTmp->framIndex = temperatureFramTransaction->searchFramIndexPosition;

					temperatureFramTransaction->startReadTransaction = false;
				}
				else
				{
					//change value of index where search must be performed
					if(temperatureFramTransaction->moveBackward == true)
						temperatureFramTransaction->searchFramIndexPosition = GUI_GetDecrementedFramIndex(temperatureFramTransaction->searchFramIndexPosition);
					else
						temperatureFramTransaction->searchFramIndexPosition = GUI_GetIncrementedFramIndex(temperatureFramTransaction->searchFramIndexPosition);
					temperatureFramTransaction->searchCounter++;

					if(temperatureFramTransaction->searchCounter == SEARCH_RADIUS)
					{
						//mark this area as broken and assign index
						temperatureFramTransaction->pointerToStructureTmp->availabilityFlag = false;
						temperatureFramTransaction->pointerToStructureTmp->notExistFlag = true;
						temperatureFramTransaction->pointerToStructureTmp->framIndex = temperatureFramTransaction->searchFramIndexPosition;

						temperatureFramTransaction->startReadTransaction = false;
					}
				}

				//unlock SPI
				ClockState.sharedSpiState = NOT_USED;
				ClockState.FramTransactionIdentifier = FRAM_ID_NOP;
			}
		}
	}/* if(BufferCursor.loadDataFlag == true || temperatureFramTransaction->startReadTransaction == true) */
}

/*****************************************************************************************
* wifiProcessFramSearchRequest() - search FRAM if appropriate request from WIFI module
* will be send. About search decide searchState variable in WifiStateType structure
* passed to function by pointer. This function work different then processReadTemperature
* because it don't search radius from previous element but search all FRAM memory. During
* search process is only loaded four first bytes of day measurement structure. Those bytes
* contain day, month, year and temperature source. If bytes will be the same like begining
* of searched structure then rest of data will be loaded. When complete day measurement
* structure will be loaded then it will be verified by checing checksum that data is
* correct. If new search request will be send function will check previous loaded structure.
* When structure will be same like searched then FRAM will not be searched. Function on
* final step generate SOME/IP response payload. This function is non blocking and must be
* call cyclically.
*
* Parameters:
* @wifiStateStructure: pointer to WifiStateType structure with data for handle all WIFI
*  module. This function use only part of fields from this structure(parts necessary
*  for load data from FRAM and data to asembly TX message).
*
*****************************************************************************************/
static void wifiProcessFramSearchRequest(WifiStateType* wifiStateStructure)
{
	switch(wifiStateStructure->searchState)
	{
	case SEARCH_NOT_REQUESTED:
		break;

	case SEARCH_REQUESTED:
		if(lockSharedSpiPort(FRAM_USAGE))
		{
			ClockState.FramTransactionIdentifier = FRAM_ID_SEARCH_TEMPERATURE;

			//initialize variable in structure necessary for search
			wifiStateStructure->readFramWasRequested = false;
			wifiStateStructure->searchFramIndex = 0;

			wifiStateStructure->searchState = SEARCH_PENDING;
		}

		break;

	case SEARCH_PENDING:
		if(wifiStateStructure->readFramWasRequested == false)
		{
			FRAM_Read(convertFramIndexToAddress(wifiStateStructure->searchFramIndex),
				DAY_MEASUREMENT_HEADER_SIZE, (uint8_t*)&wifiStateStructure->TemperatureSingleDayRecordTmp);

			wifiStateStructure->readFramWasRequested = true;
		}
		else
		{
			if(FRAM_Process())
			{
				wifiStateStructure->readFramWasRequested = false;
				wifiStateStructure->searchState = SEARCH_PENDING_READ_READY;
			}
		}

		break;

	case SEARCH_PENDING_READ_READY:
		//compare structure headers
		if((wifiStateStructure->SearchedDayMeasurementHeader.day == wifiStateStructure->TemperatureSingleDayRecordTmp.day)
			&& (wifiStateStructure->SearchedDayMeasurementHeader.month == wifiStateStructure->TemperatureSingleDayRecordTmp.month)
			&& (wifiStateStructure->SearchedDayMeasurementHeader.year == wifiStateStructure->TemperatureSingleDayRecordTmp.year)
			&& (wifiStateStructure->SearchedDayMeasurementHeader.source == wifiStateStructure->TemperatureSingleDayRecordTmp.source))
		{
			FRAM_Read(convertFramIndexToAddress(wifiStateStructure->searchFramIndex), sizeof(TemperatureSingleDayRecordType),
				(uint8_t*)&wifiStateStructure->TemperatureSingleDayRecordTmp);

			wifiStateStructure->searchState = SEARCH_HEADER_MATCH;
		}
		else
		{
			//case when all FRAM was searched and searched structure don't exist
			if(wifiStateStructure->searchFramIndex >= MAX_RECORD_IN_FRAM)
			{
				wifiStateStructure->searchedStructureExist = false;

				wifiStateStructure->searchState = SEARCH_FINISHED;
			}
			else
			{
				wifiStateStructure->searchFramIndex++;
				wifiStateStructure->searchState = SEARCH_PENDING;
			}
		}

		break;

	case SEARCH_HEADER_MATCH:
		if(FRAM_Process())
		{
			uint16_t checkSumValue = Chip_CRC_CRC16((uint16_t*)&wifiStateStructure->TemperatureSingleDayRecordTmp, (offsetof(TemperatureSingleDayRecordType, CRC16Value)/2));

			if(checkSumValue != wifiStateStructure->TemperatureSingleDayRecordTmp.CRC16Value)
			{
				wifiStateStructure->searchState = SEARCH_PENDING;
			}
			else//this case mean that searched structure is correct
			{
				//init all fields in global structure
				wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer = wifiStateStructure->TemperatureSingleDayRecordTmp;
				wifiStateStructure->temperatureStructureIsValid = true;
				wifiStateStructure->searchedStructureExist = true;

				wifiStateStructure->searchState = SEARCH_FINISHED;
			}
		}

		break;

	case SEARCH_FINISHED:

		//unlock SPI
		ClockState.sharedSpiState = NOT_USED;
		ClockState.FramTransactionIdentifier = FRAM_ID_NOP;

		//send SOME/IP message
		if(wifiStateStructure->searchedStructureExist)
		{
			uint16_t someIpPayloadSizeTmp = sizeof(TemperatureSingleDayRecordType) - (wifiStateStructure->someIpReceivedMessage.methodId * SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE);
			uint8_t* dataStructurePointerTmp = ((uint8_t*)&wifiStateStructure->TemperatureSingleDayRecordLoadedBuffer);

			if(someIpPayloadSizeTmp > SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE)
			{
				someIpPayloadSizeTmp = SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE;
			}

			wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
					wifiStateStructure->someIpReceivedMessage.serviceId,
					wifiStateStructure->someIpReceivedMessage.methodId,
					SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_OK_VALUE,
					wifiStateStructure->TxMessageTable[0].payload,
					&dataStructurePointerTmp[wifiStateStructure->someIpReceivedMessage.methodId * SOME_IP_SERVICE_DAY_MEASUREMENT_MAX_RESP_PAYLOAD_SIZE],
					someIpPayloadSizeTmp);
		}
		else//send SOME/IP response without payload - it mean that structure don't exist
		{
			wifiStateStructure->TxMessageTable[0].payloadSize = SOMEIP_CodeTxMessage(
					wifiStateStructure->someIpReceivedMessage.serviceId,
					wifiStateStructure->someIpReceivedMessage.methodId,
					SOME_IP_RESPONSE_CODE, SOME_IP_RETURN_CODE_E_OK_VALUE,
					wifiStateStructure->TxMessageTable[0].payload, NULL, 0);
		}

		wifiStateStructure->TxMessageTable[0].lockFlag = true;

		wifiStateStructure->searchState = SEARCH_NOT_REQUESTED;
		break;
	}/* switch(wifiStateStructure->searchState) */
}

/*****************************************************************************************
* calculateFurnaceTemperature() - calculate furnace temperature by add to temperature from
* sensor temperature offset set by GUI interface. If temperature will be invalid
* calculation will not be performed.
*
* Return: calculating temperature
*****************************************************************************************/
static uint16_t calculateFurnaceTemperature(void)
{
	if(ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValid)
	{
		int32_t temperatureTmp = (int32_t)(ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue)
			+ (int32_t)(ClockState.temperatureFurnaceOffset * 10);

		return ((uint16_t)temperatureTmp);
	}
	else
	{
		return INVALID_READ_SENSOR_VALUE;
	}
}

/*****************************************************************************************
* processAlarm() - function decide about raising alarm by checking three possible source
* of alarm condition. If alarm is raised then also PWM for buzzer is activated. Buzzer
* melody is controlled by this function.
*****************************************************************************************/
static void processAlarm(void)
{
	if(gui.active_window != &clockSettingsWindow)
	{
		if(ClockState.firstAlarmActive)
		{
			static bool firstAlarmActivationStep = true;

			if(firstAlarmActivationStep == true
				&& ClockState.firstAlarmHour == ClockState.currentTimeHour
				&& ClockState.firstAlarmMinute == ClockState.currentTimeMinute)
			{
				firstAlarmActivationStep = false;
				ClockState.firstAlarmRaised = true;
			}

			if(ClockState.firstAlarmHour != ClockState.currentTimeHour
				|| ClockState.firstAlarmMinute != ClockState.currentTimeMinute)
			{
				firstAlarmActivationStep = true;
			}
		}

		if(ClockState.secondAlarmActive)
		{
			static bool secondAlarmActivationStep = true;

			if(secondAlarmActivationStep == true
				&& ClockState.secondAlarmHour == ClockState.currentTimeHour
				&& ClockState.secondAlarmMinute == ClockState.currentTimeMinute)
			{
				secondAlarmActivationStep = false;
				ClockState.secondAlarmRaised = true;
			}

			if(ClockState.secondAlarmHour != ClockState.currentTimeHour
				|| ClockState.secondAlarmMinute != ClockState.currentTimeMinute)
			{
				secondAlarmActivationStep = true;
			}
		}
	}

	if((gui.active_window != &temperatureWindow) && ClockState.temperatureFurnaceAlarmActive)
	{
		static bool temperatureFurnaceAlarmActivationStep = true;

		if(temperatureFurnaceAlarmActivationStep && ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValid
			&& (ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue < ClockState.temperatureFurnaceAlarmThreshold))
		{
			temperatureFurnaceAlarmActivationStep = false;
			ClockState.temperatureFurnaceAlarmRaised = true;
		}

		if(ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValid
			&& (ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue > ClockState.temperatureFurnaceAlarmThreshold))
		{
			temperatureFurnaceAlarmActivationStep = true;
		}
	}

	//increment sound piezo step if at least one of three alarm is active
	if(ClockState.firstAlarmRaised == true || ClockState.secondAlarmRaised == true
		|| ClockState.temperatureFurnaceAlarmRaised)
	{
		static uint8_t soundThreadCounter = 0;

		if(soundThreadCounter >= LENGHT_OF_INCREMENT_SOUND_STEP)
		{
			Buzzer_TurnOff();
			Buzzer_SetOctave(SoundAlarmTable[ClockState.alarmSoundAnimationStep]);
			Buzzer_TurnOn();

			ClockState.alarmSoundAnimationStep++;

			if(ClockState.alarmSoundAnimationStep >= LENGHT_OF_SOUND_ALARM_TABLE)
				ClockState.alarmSoundAnimationStep = 0;

			soundThreadCounter = 0;
		}
		else
		{
			soundThreadCounter++;
		}
	}
	else
	{
		ClockState.alarmSoundAnimationStep = 0;
	}
}

/*****************************************************************************************
* Thread_Init() - initiate data for single Thread responsible for many thing like handle
* FRAM. In initialization process is set timer(responsible for cyclic call Thread), FRAM
* temperature measurement block and I2C bus expander.
*****************************************************************************************/
void Thread_Init(void)
{

	/**********************************
	*	configure measurements blocks
	***********************************/
	{
		for(uint16_t i = 0; i < NUM_OF_TEMPERATURE_SOURCE; i++)
		{
			if(ClockState.TemperatureSensorTable[i].recordTemperature == true)
			{
				initTemperatureBlock(&TemperatureSingleDay[i], &ClockState.TemperatureSensorTable[i].temperatureFramIndex, i);
			}
		}
	}
	/**********************************
	*	configure temperature FRAM structures necessary to store measurements
	***********************************/
	//temperature outside
	TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE].transactionId = FRAM_ID_MOVE_TEMPERATURE_OUTSIDE;
	TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE].previousAssignFilter = 0;
	TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE].previousAssignStore = 0;
	TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE].startTemperatureTransaction = false;
	TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE].temperatureIndex = 0;
	TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE].source = OUTSIDE_TEMPERATURE;

	//temperature inside
	TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE].transactionId = FRAM_ID_MOVE_TEMPERATURE_INSIDE;
	TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE].previousAssignFilter = 0;
	TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE].previousAssignStore = 0;
	TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE].startTemperatureTransaction = false;
	TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE].temperatureIndex = 0;
	TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE].source = INSIDE_TEMPERATURE;

	//temperature furnace
	TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE].transactionId = FRAM_ID_MOVE_TEMPERATURE_FURNACE;
	TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE].previousAssignFilter = 0;
	TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE].previousAssignStore = 0;
	TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE].startTemperatureTransaction = false;
	TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE].temperatureIndex = 0;
	TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE].source = FURNACE_TEMPERATURE;

	/**********************************
	*	configure temperature FRAM structure necessary to read measurements
	***********************************/
	TemperatureFramReadTransaction.searchCounter = 0;
	TemperatureFramReadTransaction.startReadTransaction = false;

	/**********************************
	*	configure temperature sensor
	***********************************/
	TemperatureSensor_ChoseSensor(CN6_TEMP_INSIDE);

	TemperatureSensor_StartMeasurement();

	/**********************************
	*	configure WifiState structure
	***********************************/
	WifiStateStructure.connectToApnCounter = ONE_SECONDS*30;
	WifiStateStructure.getApnCounter = ONE_SECONDS*12;

	/**********************************
	*	init timer
	***********************************/
	//connect CT16B0 to AHB
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<7);

#if 0//configuration for 12MHz
	//configure prescaller
	LPC_TIMER16_0->PR=120;

	//this register will max value of counter
	LPC_TIMER16_0->MR[0]=2000;
#else//configuration for 48MHz
	//configure prescaller
	LPC_TIMER16_0->PR=480;

	//this register will max value of counter
	LPC_TIMER16_0->MR[0]=2000;
#endif
	//Reset on MR0 and generate interrupt(set MR0R bit and MR0I)
	LPC_TIMER16_0->MCR=3;

	//Reset Timer
	LPC_TIMER16_0->TCR = 0x2;

	//Enable timer (set CEN bit)
	LPC_TIMER16_0->TCR=1;

	//set interrupt timer priority
	NVIC_SetPriority(TIMER_16_0_IRQn, 3);

	//allow for interruptions from timer
	NVIC_EnableIRQ(TIMER_16_0_IRQn);
}

/*****************************************************************************************
* Thread_Call() - function is call every 20 ms. All things inside is asynchronous
* executed(task isn't block execution for longer period).
*****************************************************************************************/
void Thread_Call(void)
{
	/**********************************
	*	read temperature from three sensors
	***********************************/
	{
		static uint16_t* tempValuePointer = &ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValue;
		static bool* tempValueValidFlagPointer = &ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValid;
		static uint8_t nextTempSensor = 1;
		static uint16_t refreshGuiCounter = (3*ONE_SECONDS);

		TemperatureSensor_Process();

		if(TemperatureSensor_CheckMeasurementStatus() == I2C_DATA_IS_READY)
		{
			*tempValuePointer = TemperatureSensor_ReturnTemperature();

			if(*tempValuePointer == INVALID_READ_SENSOR_VALUE)
				*tempValueValidFlagPointer = false;
			else
				*tempValueValidFlagPointer = true;

			//calculate furnace temperature using offset value
			if(nextTempSensor == 3)
				*tempValuePointer = calculateFurnaceTemperature();
		}

		if (TemperatureSensor_CheckMeasurementStatus() == I2C_WAITING_FOR_REQUEST)
		{
			nextTempSensor++;
			if(nextTempSensor == 4)
				nextTempSensor = 1;

			switch(nextTempSensor)
			{
			case 1:
				tempValuePointer = &ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValue;
				tempValueValidFlagPointer = &ClockState.TemperatureSensorTable[INSIDE_TEMPERATURE].temperatureValid;
				TemperatureSensor_ChoseSensor(CN6_TEMP_INSIDE);
				break;

			case 2:
				tempValuePointer = &ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValue;
				tempValueValidFlagPointer = &ClockState.TemperatureSensorTable[OUTSIDE_TEMPERATURE].temperatureValid;
				TemperatureSensor_ChoseSensor(CN5_TEMP_OUTSIDE);
				break;

			case 3:
				tempValuePointer = &ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValue;
				tempValueValidFlagPointer = &ClockState.TemperatureSensorTable[FURNACE_TEMPERATURE].temperatureValid;

				TemperatureSensor_ChoseSensor(CN4_TEMP_FURNACE);
				break;

			default:
				break;
			}

			TemperatureSensor_StartMeasurement();
		}

		//refresh GUI temp values every 5s
		if(refreshGuiCounter >= (5*ONE_SECONDS))
		{
			if(ClockState.refreshTemperatureValues == WAITING_FOR_REQUEST)
			{
				ClockState.refreshTemperatureValues = REFRESH;
			}
			else if(ClockState.refreshTemperatureValues == REFRESH_PERFORMED)
			{
				refreshGuiCounter = 0;
				ClockState.refreshTemperatureValues = WAITING_FOR_REQUEST;
			}
		}
		else
		{
			refreshGuiCounter++;
		}
	}

	/**********************************
	*	move temperature to FRAM
	***********************************/
	processWriteTemperature(&TemperatureFramTransactionSensorTable[OUTSIDE_TEMPERATURE]);

	processWriteTemperature(&TemperatureFramTransactionSensorTable[INSIDE_TEMPERATURE]);

	processWriteTemperature(&TemperatureFramTransactionSensorTable[FURNACE_TEMPERATURE]);

	/**********************************
	*	check button responsible for Factory reset
	***********************************/
	static bool framClearFlag = false;

	{
		static uint16_t framClearCounter = 0;

		if(GPIO_GetState(RESET_BUTTON_GPIO_PORT, RESET_BUTTON_GPIO_PIN) == false)
			framClearCounter++;
		else
			framClearCounter = 0;

		if( ((framClearCounter >= (ONE_SECONDS*3)) || (ClockState.factoryResetViaGui == true))
			&& lockSharedSpiPort(FRAM_USAGE))
		{
			ClockState.FramTransactionIdentifier = FRAM_ID_FACTORY_RESET;

			framClearFlag = true;
		}

		//clear two block in FRAM memory
		if(framClearFlag && (ClockState.FramTransactionIdentifier == FRAM_ID_FACTORY_RESET))
		{
			static bool startWriteSequence = true;
			static uint8_t indexOfBlock = 0;

			if(startWriteSequence)
			{
				FRAM_Write(CLEAR_BLOCK_SIZE*indexOfBlock, CLEAR_BLOCK_SIZE, clearFramData);
				startWriteSequence = false;
			}

			if(FRAM_Process() == true)
			{
				indexOfBlock++;
				startWriteSequence = true;
			}

			if(indexOfBlock == MAX_INDEX)
			{
				NVIC_SystemReset();
			}
		}
	}
	/**********************************
	*	store clock state structure
	***********************************/
	{
		static uint16_t clockStateToFramCounter = 0;
		static uint8_t blockNumberInFram = 0;

		if((clockStateToFramCounter >= ONE_SECONDS) && (framClearFlag == false))
		{
			if(lockSharedSpiPort(FRAM_USAGE))
			{
				ClockState.FramTransactionIdentifier = FRAM_ID_CLOCKSTATE;

				//copy clockState to local buffer
				ClockStateFramBuffer = ClockState;

				//calculate crc16
				ClockStateFramBuffer.CRC16Value = Chip_CRC_CRC16((uint16_t*)&ClockStateFramBuffer, (offsetof(ClockStateType, CRC16Value)/2));

				if(blockNumberInFram == 0)
				{
					//start transmision
					FRAM_Write(FRAM_CLOCK_STATE_FIRST_COPY, sizeof(ClockStateFramBuffer),
							(uint8_t*)&ClockStateFramBuffer);
					blockNumberInFram = 1;
				}
				else
				{
					FRAM_Write(FRAM_CLOCK_STATE_SECOND_COPY, sizeof(ClockStateFramBuffer),
							(uint8_t*)&ClockStateFramBuffer);
					blockNumberInFram = 0;
				}
			}
			else if((ClockState.sharedSpiState == FRAM_USAGE)
				&& (ClockState.FramTransactionIdentifier == FRAM_ID_CLOCKSTATE))
			{
				if(FRAM_Process() == true)
				{
					clockStateToFramCounter = 0;
					ClockState.sharedSpiState = NOT_USED;
					ClockState.FramTransactionIdentifier = FRAM_ID_NOP;
				}
			}
		}
		else
		{
			clockStateToFramCounter++;
		}
	}
	/**********************************
	*	GUI manager
	***********************************/
	{
		if(ClockState.refreshGuiCounter <= 10*ONE_SECONDS)
		{
			ClockState.refreshGuiCounter++;
		}
	}
	/**********************************
	*	copy temperature data to read buffer
	***********************************/
	processReadTemperature(&TemperatureFramReadTransaction);

	/**********************************
	*	alarm
	***********************************/
	processAlarm();

	/**********************************
	*	wifi
	***********************************/
	{
		if(ClockState.wifiReady == false)
		{
			WIFI_Init();
		}
		else
		{
			WIFI_Process(&WifiStateStructure);

			wifiProcessFramSearchRequest(&WifiStateStructure);
		}
	}
}

/*****************************************************************************************
* TIMER16_0_IRQHandler() - function for handle microcontroller interuption from timer,
*****************************************************************************************/
void TIMER16_0_IRQHandler(void)
{
	Thread_Call();

	LPC_TIMER16_0->IR = 1;
}
