/*
 * thermostat.h
 *
 *  Created on: 2 апр. 2016
 *      Author: shurik
 */

#ifndef LIB_THERMOSTAT_THERMOSTAT_H_
#define LIB_THERMOSTAT_THERMOSTAT_H_
#include <SmingCore/SmingCore.h>
#include <tempsensors.h>

#ifndef ONSTATECHANGEDELEGATE_TYPE_DEFINED
#define ONSTATECHANGEDELEGATE
typedef Delegate<void(uint8_t state)> onStateChangeDelegate;
#endif

namespace ThermostatMode
{
	const uint8_t HEATING=1;
	const uint8_t COOLING=0;
}

const uint8_t maxInvalidGetTemp = 10; // max unhealthy getTemp before we assume tempsensor lost

class ThermostatClass
{
public:
	ThermostatClass(TempSensors &tempSensors, uint8_t mode = ThermostatMode::HEATING, uint8_t invalidDefaultState = true, String name = "Thermostat", uint16_t refresh = 4000);
	void start();
	void stop();
	float getTargetTemp() { return _targetTemp / 100; };
	void setTargetTemp(float targetTemp) { _targetTemp = (uint16_t)targetTemp * 100; };
	float getTargetTempDelta() { return _targetTempDelta / 100; };
	void setTargetTempDelta(float targetTempDelta) { _targetTempDelta = (uint16_t)targetTempDelta / 100; };
	uint8_t getState() { return _state; };
	void onStateChange(onStateChangeDelegate delegateFunction);
//	void onStateCfg(HttpRequest &request, HttpResponse &response);
private:
	void _check();
	String _name; // some text description of thermostat
	uint8_t _active; //thermostat active (true), ON,  works, updates, changes its _state or turned OFF
	uint8_t _state; // thermostat state on (true) or off (false)
	uint8_t _mode; // thermostat mode HEATING = true or COOLING = false
	uint16_t _targetTemp = 2300; //target temperature for manual mode MULTIPLE BY 100
	uint16_t _targetTempDelta = 50; //delta +- for both _targetTemp and manualTargetTemp MULTIPLE BY 100
	uint16_t _refresh; // thermostat update interval
	Timer _refreshTimer; // timer for thermostat update
	TempSensors *_tempSensors;
//	uint8_t	_sensorId;
	onStateChangeDelegate _onChangeState = nullptr;
	uint8_t _invalidDefaultState = true;
	uint8_t _tempSensorValid = maxInvalidGetTemp; // if more than zero we STILL trust tempSensor temperature if less zero NOT trust

};

#endif /* LIB_THERMOSTAT_THERMOSTAT_H_ */
