/*
 * thermostat.cpp
 *
 *  Created on: 2 апр. 2016
 *      Author: shurik
 */

#include <thermostat.h>

//  ThermostatClass

ThermostatClass::ThermostatClass(TempSensors &tempSensors, uint8_t mode, uint8_t invalidDefaultState, String name, uint16_t refresh)
{
	_tempSensors = &tempSensors;
	_name = name;
	_refresh = refresh;
	_state = false;
	_active = true;
	_mode = mode;
	_invalidDefaultState = invalidDefaultState;
//	_loadBinConfig();
}

void ThermostatClass::start()
{
	_refreshTimer.initializeMs(_refresh, TimerDelegate(&ThermostatClass::_check, this)).start(true);
	if (_onChangeState)
	{
		Serial.printf("Start - set init state via delegate\n");
		_onChangeState(_state);
	}
}

void ThermostatClass::stop()
{
	_refreshTimer.stop();
//	_state = false;
}

void ThermostatClass::onStateChange(onStateChangeDelegate delegateFunction)
{
	_onChangeState = delegateFunction;
}

void ThermostatClass::_check()
{
	float currTemp = _tempSensors->getTemp();

	uint8_t prevState = _state;

	if (_tempSensors->isValid())
	{
		_tempSensorValid = maxInvalidGetTemp;
	}
	else if (_tempSensorValid > 0)
	{
		_tempSensorValid--;
		Serial.printf("Name: %s - TEMPSENSOR ERROR!, %d\n", _name.c_str(), _tempSensorValid);
	}

	if (!_tempSensorValid)
	{
		_state = _invalidDefaultState; // If we lost tempsensor we set thermostat to Default Invalid State
		if (_onChangeState)
		{
			Serial.printf("We lost tempsensor! - set invalidDefaultstate via delegate!\n");
			_onChangeState(_state);
		}
		Serial.printf("Name: %s - TEMPSENSOR ERROR! - WE LOST IT!\n", _name.c_str());
	}
	else
	{
		if (currTemp >= (float)(_targetTemp / 100.0) + (float)(_targetTempDelta / 100.0))
			_state = !(_mode);
		if (currTemp <= (float)(_targetTemp / 100.0) - (float)(_targetTempDelta / 100.0))
			_state = _mode;
	}
	Serial.printf(" State: %s\n", _state ? "true" : "false");
	if (prevState != _state && _onChangeState)
	{
		Serial.printf("onChangeState Delegate/CB called!\n");
		_onChangeState(_state);
	}
}

void ThermostatClass::onHttpConfig(HttpRequest &request, HttpResponse &response)
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

				if (root["targetTemp"].success()) // Settings
				{
//					_targetTemp = ((float)(root["targetTemp"]) * 100);
					_targetTemp = root["targetTemp"];
					needSave = true;
				}
				if (root["targetTempDelta"].success()) // Settings
				{
//					_targetTempDelta = ((float)(root["targetTempDelta"]) * 100);
					_targetTempDelta = root["targetTempDelta"];
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

//			json["name"] = _name;
//			json["active"] = _active;
//			json["state"] = _state;
			json["targetTemp"] = _targetTemp;
			json["targetTempDelta"] = _targetTempDelta;

			response.setHeader("Access-Control-Allow-Origin", "*");
			response.sendJsonObject(stream);
		}
}

void ThermostatClass::_saveBinConfig()
{
	Serial.printf("Try to save bin cfg..\n");
	file_t file = fileOpen("tstat" + _name, eFO_CreateIfNotExist | eFO_WriteOnly);
	fileWrite(file, &_targetTemp, sizeof(_targetTemp));
	fileWrite(file, &_targetTempDelta, sizeof(_targetTempDelta));
	fileClose(file);
}

void ThermostatClass::_loadBinConfig()
{
	Serial.printf("Try to load bin cfg..\n");
	if (fileExist("tstat" + _name))
	{
		Serial.printf("Will load bin cfg..\n");
		file_t file = fileOpen("tstat" + _name, eFO_ReadOnly);
		fileSeek(file, 0, eSO_FileStart);
		fileRead(file, &_targetTemp, sizeof(_targetTemp));
		fileRead(file, &_targetTempDelta, sizeof(_targetTempDelta));
		fileClose(file);
	}
}
