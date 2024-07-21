//inspiration from https://www.youtube.com/watch?v=xDdul0FhAAY&list=PLCrgFeG6pwQmdbB6l3ehC8oBBZbatVoz3&index=14&ab_channel=BelaPlatform

#pragma once

class Ramp {
	
	public:
	
		Ramp();
		Ramp(float sampleRate);
		void setSampleRate(float sampleRate) {_sampleRate = sampleRate;}
		void setValue(float value);
		void rampTo(float value, float time);
		float process();
		bool finished() {return _counter == 0;}
		~Ramp() {}
	
	private:
	
		float _sampleRate;
		float _currentValue;
		float _increment;
		unsigned int _counter;
	
};
