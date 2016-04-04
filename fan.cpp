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
	_fanRelay->setState(false);
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
		_fanTimer.initializeMs(_startDuration * 6000, TimerDelegate(&FanClass::_modeStartEnd, this)).start(false);
	}
}

void FanClass::_modeStartEnd()
{
	Serial.printf("START Finished\n");
	_fanRelay->setState(false);
	_thermostat->start();
	//TODO: CHANGE THIS LATER FOR 60000!!!
	_fanTimer.initializeMs(_periodicInterval * 6000, TimerDelegate(&FanClass::_pereodic, this)).start(false);
	_mode = FanMode::RUN;
}

void FanClass::_pereodic()
{
	Serial.printf("PREIODIC START\n");
	_periodicStartTemp = _tempSensor->getTemp();
	_thermostat->stop();
	_fanRelay->setState(true);
	//TODO: CHANGE THIS LATER FOR 60000!!!
	_fanTimer.initializeMs(_periodicDuration * 6000, TimerDelegate(&FanClass::_pereodicEnd, this)).start(false);
	_mode = FanMode::PERIODIC;
}

void FanClass::_pereodicEnd()
{
	Serial.printf("PREIODIC END\n");
	float _periodicEndTemp = _tempSensor->getTemp();
	Serial.printf("Periodic end - startTemp: ");Serial.print(_periodicStartTemp);Serial.printf(" endTemp ");Serial.println(_periodicEndTemp);
	if (_periodicEndTemp - _periodicStartTemp > (float)(_periodicTempDelta / 100))
	{
		_periodicCounter = maxLowTempCount;
	} else if (_periodicCounter > 0)
	{
		Serial.printf("Seems like no wood left, giving another chance!\n");
		_periodicCounter--;
	}
	if (!_periodicCounter)
	{
		Serial.printf("No wood left! going idle!\n");
		_fanRelay->setState(false);
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
		_fanTimer.initializeMs(_periodicInterval * 6000, TimerDelegate(&FanClass::_pereodic, this)).start(false);
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
		_fanTimer.initializeMs(_startDuration * 6000, TimerDelegate(&FanClass::_modeStopEnd, this)).start(false);
	}
}

void FanClass::_modeStopEnd()
{
	Serial.printf("STOP Finished\n");
	_fanRelay->setState(false);
	_thermostat->stop();
	_fanTimer.stop();
	_mode = FanMode::IDLE;
}
