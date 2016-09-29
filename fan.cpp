/*
 * FanClass.cpp
 *
 *  Created on: 3 апр. 2016
 *      Author: shurik
 */

#include <fan.h>

// FanClass

FanClass::FanClass(TempSensors &tempSensor, ThermostatClass &thermostat, BinOutClass &fanRelay)
{
	_tempSensor = &tempSensor;
	_thermostat = &thermostat;
	_fanRelay = &fanRelay;
//	_startButton->state.onChange(onStateChangeDelegate(&FanClass::_modeStart, this));
//	_stopButton->state.onChange(onStateChangeDelegate(&FanClass::_modeStop, this));

	state.onChange(onStateChangeDelegate(&FanClass::_enable, this));

//	_fanRelay->setState(false); //No need, disabling thermostat with default stop will turn off fan
	_thermostat->state.onChange(onStateChangeDelegate(&BinStateClass::set, &_fanRelay->state));
	_thermostat->state.onChange(onStateChangeDelegate(&FanClass::_checkerEnable, this));
	_thermostat->stop();
};

void FanClass::_enable(uint8_t enableState)
{
	if (enableState)
	{
		_modeStart(enableState);
	}
	else
	{
		_modeStop(!enableState);
	}
}

void FanClass::_modeStart(uint8_t state)
{
	if (state)
	{
		_mode = FanMode::START;
		Serial.printf("START Button pressed\n");
		_thermostat->stop(false);
		_fanRelay->state.set(true);
		//TODO: CHANGE THIS LATER FOR 60000!!!
		_fanTimer.initializeMs(_startDuration * 60000, TimerDelegate(&FanClass::_modeStartEnd, this)).start(false);
	}
}

void FanClass::_modeStartEnd()
{
	Serial.printf("START Finished\n");
	_fanRelay->state.set(false);
	_thermostat->start();
	_periodicCounter = _maxLowTempCount; // Reset pereodicCounter
//	_fanTimer.initializeMs(_periodicInterval * 60000, TimerDelegate(&FanClass::_pereodic, this)).start(false);
	_mode = FanMode::RUN;
}

void FanClass::_pereodic()
{
	Serial.printf("PREIODIC START\n");
	_thermostat->stop(false);
	_fanRelay->state.set(true);
	_fanTimer.initializeMs(_periodicDuration * 60000, TimerDelegate(&FanClass::_pereodicEnd, this)).start(false);
	_mode = FanMode::PERIODIC;
}

void FanClass::_pereodicEnd()
{
	Serial.printf("PERIODIC END - GO TO RUN MODE\n");
	_fanRelay->state.set(false);
	_thermostat->start();
	_fanTimer.initializeMs(_periodicInterval * 60000, TimerDelegate(&FanClass::_pereodic, this)).start(false);
	_mode = FanMode::RUN;
}

void FanClass::_modeStop(uint8_t state)
{
	if (state)
	{
		_mode = FanMode::STOP;
		Serial.printf("STOP Button pressed\n");
		_thermostat->stop(false);
		_fanRelay->state.set(true);
		_fanTimer.initializeMs(_startDuration * 60000, TimerDelegate(&FanClass::_modeStopEnd, this)).start(false);

	}
}

void FanClass::_modeStopEnd()
{
	Serial.printf("STOP Finished\n");
//	_fanRelay->setState(false); //No need, disabling thermostat with default stop will turn off fan
	_fanRelay->state.set(false);
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
				if (root["checkerInterval"].success())
				{
					_checkerInterval = root["checkerInterval"];
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
			json["checkerInterval"] = _checkerInterval;

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
	fileWrite(file, &_checkerInterval, sizeof(_checkerInterval));
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
		fileRead(file, &_checkerInterval, sizeof(_checkerInterval));
		fileClose(file);
	}
}

void FanClass::_checkerEnable(uint8_t enabled)
{
	if (enabled)
	{
		_checkerStart();
	}
	else
	{
		_checkerStop();
	}
}

void FanClass::_checkerStart()
{
	Serial.printf("Checker STARTED!\n");
	_chekerMaxTemp = _tempSensor->getTemp();
	_checkerTimer.initializeMs(_checkerInterval * 60000, TimerDelegate(&FanClass::_checkerCheck, this)).start(true);
}

void FanClass::_checkerStop()
{
	Serial.printf("Checker STOPPED!\n");
	_checkerTimer.stop();
}

void FanClass::_checkerCheck()
{
	float _checkerCheckTemp = _tempSensor->getTemp();
	if (_checkerCheckTemp > _chekerMaxTemp)
	{
		_chekerMaxTemp = _checkerCheckTemp;
	}
	Serial.printf("Checker CHECK- startTemp: ");Serial.print(_chekerMaxTemp);Serial.printf(" endTemp ");Serial.print(_checkerCheckTemp);
	Serial.printf(" _periodicTempDelta: ");Serial.println((float)(_periodicTempDelta / 100.0));
	if (_checkerCheckTemp - _chekerMaxTemp <= (float)(_periodicTempDelta / 100.0))
	{
		Serial.printf("No wood left! Go to IDLE!\n");
		_thermostat->stop();
//		_fanRelay->setState(false);
		_fanTimer.stop();
		_mode = FanMode::IDLE;
	}
	else
	{
		Serial.printf("We still have wood! WORKING!\n");
	}
}
