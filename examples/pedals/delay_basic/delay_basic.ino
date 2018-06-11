#include "audioFX.h"
#include "pedalControls.h"
#include "delay.h"
#include "ak4558.h"

using namespace FX;

#define ADC_SCALE 2097152

static RAMB q31 outputDataL[AUDIO_BUFSIZE], scratch[AUDIO_BUFSIZE], scratch2[AUDIO_BUFSIZE];

#define DELAY_SIZE (AUDIO_BUFSIZE*256)
static L2DATA q31 delayBuf[DELAY_SIZE];
struct delayLine *line;
struct delayTap *tap;

uint16_t delayMixLast = 0, delayFeedbackLast = 0, delayTimeLast = 0;
q31 delayMix = 0, delayFeedback = 0;

ak4558 iface;

void audioHook(q31 *data)
{
	deinterleave(data, outputDataL, scratch);

	delayPop(tap, scratch);
	copy(scratch2, outputDataL);
	mix(scratch2, scratch, delayFeedback);
	delayPush(line, scratch2);

	mix(outputDataL, scratch, delayMix);

	interleave(data, outputDataL, outputDataL);
}

void setup(){
	line = initDelayLine(delayBuf, DELAY_SIZE);
	tap = initDelayTap(line, 0);

	controls.begin();

	iface.begin();

	//begin fx processor
	fx.begin();

	//set the function to be called when a buffer is ready
	fx.setHook(audioHook);
}

void loop(){
	if(controls.state.adcPrimary[0] != delayMixLast){
		delayMixLast = controls.state.adcPrimary[0];
		delayMix = (q31)delayMixLast * ADC_SCALE;
	}
	if(controls.state.adcPrimary[1] != delayFeedbackLast){
		delayFeedbackLast = controls.state.adcPrimary[1];
		delayFeedback = (q31)delayFeedbackLast * ADC_SCALE;
	}
	if(controls.state.adcPrimary[2] != delayTimeLast){
		delayTimeLast = controls.state.adcPrimary[2];
		_delay_move(tap, (255 - (delayTimeLast>>2)) * AUDIO_BUFSIZE);
	}
	delay(17);
	__asm__ volatile("IDLE;");
}