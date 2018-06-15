#include <Arduino.h>
#include "audioFX.h"
#include "pedalControls.h"
#include "delay.h"
#include "lfo.h"
#include "ak4558.h"
#include "tilt.h"

using namespace FX;

#define MIX_KNOB 0
#define FEEDBACK_KNOB 1
#define TIME_KNOB 2
#define MULT_KNOB 3
#define SPACING_KNOB 4
#define FILTER_KNOB 5

#define FILTER_GAIN_MAX 6.0

#define ADC_SCALE 2097152

static RAMB q31 outputDataL[AUDIO_BUFSIZE],
				outputDataR[AUDIO_BUFSIZE],
				mixL[AUDIO_BUFSIZE],
				mixR[AUDIO_BUFSIZE],
				lfoData[AUDIO_BUFSIZE],
				scratch[AUDIO_BUFSIZE],
				scratch2[AUDIO_BUFSIZE];

#define NUM_TAPS 4
#define NUM_DELAY_BLOCKS 512
#define DELAY_SIZE (AUDIO_BUFSIZE*NUM_DELAY_BLOCKS)
static L2DATA q31 delayBuf[DELAY_SIZE];
struct delayLine *line;
struct delayTap *taps[NUM_TAPS];

static const q31 tapRateMin[] = {
		0x7FFFFFFF, _F(.1), _F(.25), _F(.5)
};

static const q31 tapRateMax[] = {
		0x7FFFFFFF, _F(.75), _F(.8), _F(.9)
};

uint16_t delayMixLast = 0,
		 delayFeedbackLast = 0,
		 delayTimeLast = 0,
		 multLast = 0,
		 spacingLast = 0,
		 filterLast = 0;

q31 delayMix = 0, delayFeedback = 0, delayMult = 0, tapSpacing = 0;

float filterGain = 0;

struct lfo *pans[NUM_TAPS];
static const q28 panRates[] = {
		_F28(1.0), _F28(2.3), _F28(.4), _F28(.8)
};

static const q31 panDepths[] = {
		_F(.999), _F(.999), _F(.999), _F(.999)
};

static uint32_t tapBase[NUM_TAPS] = {0, 0, 0, 0};

struct tilt *filter;

ak4558 iface;

void audioHook(q31 *data)
{
	deinterleave(data, outputDataL, outputDataR);
	copy(outputDataR, outputDataL);
	zero(mixL);
	zero(mixR);

	copy(scratch2, outputDataL);

	//scale feedback mix by mult
	q31 fbMix = _mult32x32(delayFeedback, 0x7FFFFFFF - _mult32x32(0x7FFFFFFF - _F(1./NUM_TAPS), delayMult));

	//scale feedback mix to compensate for filter gain
	fbMix = _mult32x32(fbMix, 0x7FFFFFFF - (0x7FFFFFFF/2/512*abs((int)filterLast-512)));
	for(int i=0; i<NUM_TAPS; i++){
		if(taps[i]->roc > 0) delayModulate(taps[i], scratch);
		else delayPop(taps[i], scratch);
		if(i > 0) gain(scratch, scratch, taps[i]->coeff);

		triangle(pans[i], lfoData);
		pan(scratch, lfoData, mixL, mixR);

		mix(scratch2, scratch, fbMix);
	}
	processTilt(filter, scratch2);
	delayPush(line, scratch2);

	mix(outputDataL, mixL, delayMix);
	mix(outputDataR, mixR, delayMix);

	interleave(data, outputDataL, outputDataR);
}

void setup(){
	filter = initTilt(2000);

	line = initDelayLine(delayBuf, DELAY_SIZE);
	for(int i=0; i<NUM_TAPS; i++){
		taps[i] = initDelayTap(line, 0);
		pans[i] = initLFO(panRates[i], panDepths[i]);
	}

	controls.begin();

	iface.begin();

	//begin fx processor
	fx.begin();

	//set the function to be called when a buffer is ready
	fx.setHook(audioHook);
}

void updateDelayTimes(){
	int timeMax = map(delayTimeLast, 0, 1023, 256, DELAY_SIZE);
	for(int i=0; i<NUM_TAPS; i++){
		if(i == 0) tapBase[i] = DELAY_SIZE - timeMax;
		else{
			q31 point = tapRateMin[i] + _mult32x32((tapRateMax[i] - tapRateMin[i]), tapSpacing);
			tapBase[i] = DELAY_SIZE - _mult32x32(timeMax, point);
		}
		if(taps[i]->roc == 0) _delay_move(taps[i], tapBase[i]);
		taps[i]->top = tapBase[i];
	}
}

void loop(){
	if(controls.state.adcPrimary[MIX_KNOB] != delayMixLast){
		delayMixLast = controls.state.adcPrimary[MIX_KNOB];
		delayMix = (q31)delayMixLast * ADC_SCALE;
	}
	if(controls.state.adcPrimary[FEEDBACK_KNOB] != delayFeedbackLast){
		delayFeedbackLast = controls.state.adcPrimary[FEEDBACK_KNOB];
		delayFeedback = (q31)delayFeedbackLast * ADC_SCALE;
	}
	if(controls.state.adcPrimary[TIME_KNOB] != delayTimeLast){
		delayTimeLast = controls.state.adcPrimary[TIME_KNOB];
		updateDelayTimes();
	}
	if(controls.state.adcPrimary[MULT_KNOB] != multLast){
		multLast = controls.state.adcPrimary[MULT_KNOB];
		delayMult = (q31)multLast * ADC_SCALE;
		int interval = 1023 / (NUM_TAPS - 1);
		int depthMul = 0x7FFFFFFF / interval;
		for(int i=1; i<NUM_TAPS; i++){
			int depth = multLast - interval * (i - 1);
			if(depth <= 0) taps[i]->coeff = 0;
			else if(depth >= interval) taps[i]->coeff  = 0x7FFFFFFF;
			else taps[i]->coeff  = depthMul * depth;
		}
	}
	if(controls.state.adcPrimary[SPACING_KNOB] != spacingLast){
		spacingLast = controls.state.adcPrimary[SPACING_KNOB];
		tapSpacing = (q31)spacingLast * ADC_SCALE;
		updateDelayTimes();
	}
	if(controls.state.adcPrimary[FILTER_KNOB] != filterLast){
		filterLast = controls.state.adcPrimary[FILTER_KNOB];
		filterGain = FILTER_GAIN_MAX/512.0 * ((int)filterLast - 512);
		setTiltGain(filter, filterGain);
	}
	delay(17);
	__asm__ volatile("IDLE;");
}
