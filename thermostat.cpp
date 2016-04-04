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
