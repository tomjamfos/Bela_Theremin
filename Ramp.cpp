#include "Ramp.h"

Ramp::Ramp() {
	
	_currentValue = 0;
	_increment = 0;
	_counter = 0;
	_sampleRate = 1;
	
}

Ramp::Ramp(float sampleRate) {
	
	_currentValue = 0;
	_increment = 0;
	_counter = 0;
	_sampleRate = sampleRate;
	
}

void Ramp::setValue(float value) {
	
	_currentValue = value;
	_increment = 0;
	_counter = 0;
	
}

void Ramp::rampTo(float value, float time) {
	
	_increment = (value - _currentValue) / (_sampleRate * time);
	_counter = (int) (_sampleRate * time);
	
}

float Ramp::process() {
	
	if(_counter > 0) {
		_counter--;
		_currentValue += _increment;
	}
	
	return _currentValue;
	
}