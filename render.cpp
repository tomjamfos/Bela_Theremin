/*
	Digital instrument based on moog theremin.
	
	Uses Trill bar to control pitch & note on/off, Trill square to control vibrato rate and depth.
	
	LEDs are used to visualise amplitude & vibrato depth.
*/

#include <Bela.h>
#include <vector>
#include <cmath>
#include <libraries/Trill/Trill.h>
#include <libraries/Gui/Gui.h>
#include <libraries/GuiController/GuiController.h>
#include <libraries/OnePole/OnePole.h>
#include "Wavetable.h"
#include "Ramp.h"

//trill bar
Trill trillBar;
bool gBarActiveTouch = 0;
bool gBarPrevTouch = 0;
float gBarTouchLocation = 0.0;
float gBarValue = .5;

//trill square
Trill trillSqr;
float gSqrTouchSize = 0.0;
float gSqrTouchLocations[2] = {0.0, 0.0};
float gSqrXYValues[2] = {0.0, 0.0};

//smoothing filters
OnePole gSmoothingFilters[3];

//gui
Gui gGui;
GuiController gController;

//oscillators
Wavetable gOsc, gLfo;

//amplitude ramp
Ramp gRamp;

//led pins 
const unsigned int kLedPins[2] = {0, 1};

void loop(void*) {
	
	while(!Bela_stopRequested()) {
		
		//read trill bar
		trillBar.readI2C();
		//active touch
		gBarActiveTouch = trillBar.getNumTouches();
		//touch location
		gBarTouchLocation = trillBar.touchLocation(0);
		
		//read trill square
		trillSqr.readI2C();
		//touch size
		gSqrTouchSize = trillSqr.compoundTouchSize();
		gSqrTouchLocations[0] = 1.0 - trillSqr.compoundTouchLocation();
		gSqrTouchLocations[1] = trillSqr.compoundTouchHorizontalLocation();
		
		//wait before next reading
		usleep(12000);
		
	}
	
}

bool setup(BelaContext *context, void *userData) {
	
	//setup trill bar
	if(trillBar.setup(1, Trill::BAR, 0x20) != 0) {
		fprintf(stderr, "Unable to initialise trill bar\n");
		return false;
	}
	trillBar.setMinimumTouchSize(.1);
	trillBar.printDetails();
	
	//setup trill square
	if(trillSqr.setup(1, Trill::SQUARE, 0x28) != 0) {
		fprintf(stderr, "Unable to initialise Trill square\n");
		return false;
	}
	trillSqr.setMinimumTouchSize(.1);
	trillSqr.printDetails();
	
	//run loop function
	Bela_runAuxiliaryTask(loop);
	
	//setup smoothing filters
	for(unsigned int i = 0; i < 3; i++) {
		OnePole filter(4, context->audioSampleRate);
		gSmoothingFilters[i] = filter;
	}
	
	//setup gui
	gGui.setup(context->projectName);
	gController.setup(&gGui, "Theremin");
	gController.addSlider("dB", -12, -60, 0, 0);
	
	//setup oscillators
	const unsigned int wavetableSize = 1024;
	std::vector<float> wavetable;
	wavetable.resize(wavetableSize);
	for(unsigned int i = 0; i < wavetable.size(); i++) {
		wavetable[i] = sinf(2.0 * M_PI * (float) i / (float) wavetable.size());
	}
	gOsc.setup(context->audioSampleRate, wavetable);
	gLfo.setup(context->audioSampleRate, wavetable);
	
	//setup amplitude ramp
	gRamp.setSampleRate(context->audioSampleRate);
	gRamp.setValue(0);
	
	return true;
	
}

void render(BelaContext *context, void *userData) {
	
	//get dB value from gui
	float mastDB = gController.getSliderValue(0); 
	
	//convert dB to amplitude 
	float mastAmp = powf(10.0, mastDB / 20.0);
	
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		
		//update value only when bar is touched
		if(gBarActiveTouch) {gBarValue = gBarTouchLocation;}
		
		//smooth value
		float barSmoothedValue = gSmoothingFilters[0].process(gBarValue);
		
		//convert 0 - 1 range to continuous range between A5 (880Hz) & A6 (1760Hz)
		float oscFreq = 880.0 * powf(2.0, barSmoothedValue);
		
		//update square x value only when touched
		if(gSqrTouchSize) {gSqrXYValues[0] = gSqrTouchLocations[0];}
		
		gSqrXYValues[1] = gSqrTouchLocations[1];
		
		//smooth square touch locations
		float sqrXValueSmoothed = gSmoothingFilters[1].process(gSqrXYValues[0]);
		float sqrYValueSmoothed = gSmoothingFilters[2].process(gSqrXYValues[1]);
		
		//convert 0 - 1 range to 2.5Hz - 10Hz
		float lfoFreq = 2.5 * powf(2.0, sqrXValueSmoothed * 2.0);
		gLfo.setFreq(lfoFreq);
		
		//lfo oscillator output
		float lfo = gLfo.process();
		
		//apply vibrato
		oscFreq += oscFreq * lfo * .125 * sqrYValueSmoothed;
		gOsc.setFreq(oscFreq);
		
		//oscillator output
		float out = gOsc.process();
		
		if(gBarActiveTouch != gBarPrevTouch) {
			//when touch is first made, start linear ramp to on
			if(gBarActiveTouch) {gRamp.rampTo(1, .8);}
			//when touch is removed, start linear ramp to off
			else {gRamp.rampTo(0, .4);}
		}
		gBarPrevTouch = gBarActiveTouch;
		
		//ramp output
		float amp = gRamp.process();
		
		//scale oscillator by ramp value
		out *= amp;
		
		//scale output by gui slider
		out *= mastAmp;
		out *= .85;
		
		//send to audio out
		for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			audioWrite(context, n, channel, out);
		}
		
		//when oscillator has faded out turn bar led off
		if(!gBarActiveTouch && gRamp.finished()) {analogWriteOnce(context, n, kLedPins[0], 0);}
		
		//bar led intensity according to amplitude
		else {
			float barLedValue = map(amp, 0, 1, .45, 1);
			analogWrite(context, n, kLedPins[0], barLedValue);
		}
		
		//square led intensity according to square y-position 
		if(gSqrTouchSize) {
			float sqrLedValue = map(sqrYValueSmoothed, 0, 1, .45, 1);
			analogWrite(context, n, kLedPins[1], sqrLedValue);
		}
		
		//when no touch turn off
		else {analogWriteOnce(context, n, kLedPins[1], 0);}
		
	}
	
}

void cleanup(BelaContext *context, void *userData) {
	
}