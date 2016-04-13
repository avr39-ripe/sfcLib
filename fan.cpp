/*
 * FanClass.cpp
 *
 *  Created on: 3 апр. 2016
 *      Author: shurik
 */

#include <fan.h>

// FanClass

FanClass::FanClass(TempSensors &tempSensor, ThermostatClass &thermostat, BinInClass &startButton, BinInClass &stopButton, BinOutClass &fanRelay)
{
	_tempSensor = &tempSensor;
	_thermostat = &thermostat;
	_startButton = &startButton;
	_stopButton = &stopButton;
	_fanRelay = &fanRelay;
	_startButton->onStateChange(onStateChangeDelegate(&FanClass::_modeStart, this));
	_stopButton->onStateChange(onStateChangeDelegate(&FanClass::_modeStop, this));
//	_fanRelay->setState(false); //No need, disabling thermostat with default stop will turn off fan
	_thermostat->onStateChange(onStateChangeDelegate(&BinOutClass::setState, _fanRelay));
	_thermostat->stop();
};

void FanClass::_modeStart(uint8_t state)
{
	if (state)
	{
		_mode = FanMode::START;
		Serial.printf("START Button pressed\n");
		_fanRelay->setState(true);
		//TODO: CHANGE THIS LATER FOR 60000!!!
		_fanTimer.initializeMs(_startDuration * 60000, TimerDelegate(&FanClass::_modeStartEnd, this)).start(false);
	}
}

void FanClass::_modeStartEnd()
{
	Serial.printf("START Finished\n");
	_fanRelay->setState(false);
	_thermostat->start();
	_periodicCounter = _maxLowTempCount; // Reset pereodicCounter
	//TODO: CHANGE THIS LATER FOR 60000!!!
	_fanTimer.initializeMs(_periodicInterval * 60000, TimerDelegate(&FanClass::_pereodic, this)).start(false);
	_mode = FanMode::RUN;
}

void FanClass::_pereodic()
{
	Serial.printf("PREIODIC START\n");
	_periodicStartTemp = _tempSensor->getTemp();
	_thermostat->stop(false);
	_fanRelay->setState(true);
	//TODO: CHANGE THIS LATER FOR 60000!!!
	_fanTimer.initializeMs(_periodicDuration * 60000, TimerDelegate(&FanClass::_pereodicEnd, this)).start(false);
	_mode = FanMode::PERIODIC;
}

void FanClass::_pereodicEnd()
{
	Serial.printf("PREIODIC END\n");
	float _periodicEndTemp = _tempSensor->getTemp();
	Serial.printf("Periodic end - startTemp: ");Serial.print(_periodicStartTemp);Serial.printf(" endTemp ");Serial.println(_periodicEndTemp);
	if (_periodicEndTemp - _periodicStartTemp > (float)(_periodicTempDelta / 100))
	{
		_periodicCounter = _maxLowTempCount;
	} else if (_periodicCounter > 0)
	{
		Serial.printf("Seems like no wood left, giving another chance!\n");
		_periodicCounter--;
	}
	if (!_periodicCounter)
	{
		Serial.printf("No wood left! going idle!\n");
//		_fanRelay->setState(false); //No need, disabling thermostat with default stop will turn off fan
		_thermostat->stop();
		_fanTimer.stop();
		_mode = FanMode::IDLE;
	}
	else
	{
		Serial.printf("PERIODIC - GO TO RUN MODE\n");
		_fanRelay->setState(false);
		_thermostat->start();
		//TODO: CHANGE THIS LATER FOR 60000!!!
		_fanTimer.initializeMs(_periodicInterval * 60000, TimerDelegate(&FanClass::_pereodic, this)).start(false);
		_mode = FanMode::RUN;
	}
}

void FanClass::_modeStop(uint8_t state)
{
	if (state)
	{
		_mode = FanMode::STOP;
		Serial.printf("STOP Button pressed\n");
		_fanRelay->setState(true);
		//TODO: CHANGE THIS LATER FOR 60000!!!
		_fanTimer.initializeMs(_startDuration * 60000, TimerDelegate(&FanClass::_modeStopEnd, this)).start(false);
	}
}

void FanClass::_modeStopEnd()
{
	Serial.printf("STOP Finished\n");
//	_fanRelay->setState(false); //No need, disabling thermostat with default stop will turn off fan
	_thermostat->stop();
	_fanTimer.stop();
	_mode = FanMode::IDLE;
}

void FanClass::onHttpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
		{
			if (request.getBody() == NULL)
			{
				debugf("NULL bodyBuf");
				return;
			}
			else
			{
				uint8_t needSave = false;
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(request.getBody());
				root.prettyPrintTo(Serial); //Uncomment it for debuging

				if (root["startDuration"].success())
				{
					_startDuration = root["startDuration"];
					needSave = true;
				}
				if (root["stopDuration"].success())
				{
					_stopDuration = root["stopDuration"];
					needSave = true;
				}
				if (root["periodicInterval"].success())
				{
					_periodicInterval = root["periodicInterval"];
					needSave = true;
				}
				if (root["periodicDuration"].success())
				{
					_periodicDuration = root["periodicDuration"];
					needSave = true;
				}
				if (root["periodicTempDelta"].success())
				{
					_periodicTempDelta = root["periodicTempDelta"];
					needSave = true;
				}
				if (root["maxLowTempCount"].success())
				{
					_maxLowTempCount = root["maxLowTempCount"];
					needSave = true;
				}
				if (needSave)
				{
					_saveBinConfig();
				}
			}
		}
		else
		{
			JsonObjectStream* stream = new JsonObjectStream();
			JsonObject& json = stream->getRoot();

			json["startDuration"] = _startDuration;
			json["stopDuration"] = _stopDuration;
			json["periodicInterval"] = _periodicInterval;
			json["periodicDuration"] = _periodicDuration;
			json["periodicTempDelta"] = _periodicTempDelta;
			json["maxLowTempCount"] = _maxLowTempCount;

			response.setHeader("Access-Control-Allow-Origin", "*");
			response.sendJsonObject(stream);
		}
}

void FanClass::_saveBinConfig()
{
	Serial.printf("Try to save bin cfg..\n");
	file_t file = fileOpen("fan.config", eFO_CreateIfNotExist | eFO_WriteOnly);
	fileWrite(file, &_startDuration, sizeof(_startDuration));
	fileWrite(file, &_stopDuration, sizeof(_stopDuration));
	fileWrite(file, &_periodicInterval, sizeof(_periodicInterval));
	fileWrite(file, &_periodicDuration, sizeof(_periodicDuration));
	fileWrite(file, &_periodicTempDelta, sizeof(_periodicTempDelta));
	fileWrite(file, &_maxLowTempCount, sizeof(_maxLowTempCount));
	fileClose(file);
}

void FanClass::_loadBinConfig()
{
	Serial.printf("Try to load bin cfg..\n");
	if (fileExist("fan.config"))
	{
		Serial.printf("Will load bin cfg..\n");
		file_t file = fileOpen("fan.config", eFO_ReadOnly);
		fileSeek(file, 0, eSO_FileStart);
		fileRead(file, &_startDuration, sizeof(_startDuration));
		fileRead(file, &_stopDuration, sizeof(_stopDuration));
		fileRead(file, &_periodicInterval, sizeof(_periodicInterval));
		fileRead(file, &_periodicDuration, sizeof(_periodicDuration));
		fileRead(file, &_periodicTempDelta, sizeof(_periodicTempDelta));
		fileRead(file, &_maxLowTempCount, sizeof(_maxLowTempCount));
		fileClose(file);
	}
}
