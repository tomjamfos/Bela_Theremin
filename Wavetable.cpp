#include "Wavetable.h"

Wavetable::Wavetable(float sampleRate, std::vector<float>& table, bool useInterp) {
	
	setup(sampleRate, table, useInterp);
	
}

void Wavetable::setup(float sampleRate, std::vector<float>& table, bool useInterp) {
	
	_inverseSampleRate = 1.0 / sampleRate;
	_table = table;
	_useInterp = useInterp;
	_readPointer = 0;
	
}

float Wavetable::process() {
	
	float out = 0;
	
	if(!_table.size()) {return out;}
	
	_readPointer += _table.size() * _freq * _inverseSampleRate;
	while(_readPointer >= _table.size()) {_readPointer -= _table.size();}
	
	if(_useInterp) {
		int indexBelow = _readPointer;
		int indexAbove = indexBelow + 1;
		if(indexAbove >= _table.size()) {indexAbove = 0;}
		float fractAbove = _readPointer - indexBelow;
		float fractBelow = 1.0 - fractAbove;
		out = fractBelow * _table[indexBelow] + fractAbove * _table[indexAbove];
	}
	else {out = _table[(int) _readPointer];}
	
	return out;
	
}