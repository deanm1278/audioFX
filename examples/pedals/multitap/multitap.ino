#include <Arduino.h>
#include "audioFX.h"
#include "pedalControls.h"
#include "delay.h"
#include "lfo.h"
#include "ak4558.h"
#include "tilt.h"
#include <Adafruit_NeoPixel.h>

using namespace FX;

#define MIX_KNOB 0
#define FEEDBACK_KNOB 1
#define TIME_KNOB 2
#define MULT_KNOB 3
#define SPACING_KNOB 4
#define FILTER_KNOB 5

#define MOD_DEPTH_KNOB 0
#define MOD_RATE_KNOB 1
#define PHASER_RATE_KNOB 2
#define PHASER_INTENSITY_KNOB 3
#define AP_ROC_KNOB 4
#define AP_MIX_KNOB 5

#define FILTER_GAIN_MAX 6.0

//the number of allpass filters in the phaser
#define NUM_AP 6

q31 phaserLast[NUM_AP];
q31 phaserk[NUM_AP] = {
    _F(.78), _F(.64), _F(.7),
    _F(.89), _F(.74), _F(.86)
};

//this will be a simple triangle LFO
struct lfo *phaserLFO;

static q31 phaserout = 0, phaserFeedback = 0, phaserDepth = 0;

#define DELAY_MAX_ORDER (11)
#define AP1_DELAY ((1UL << DELAY_MAX_ORDER)+AUDIO_BUFSIZE)
#define AP2_DELAY ((1UL << (DELAY_MAX_ORDER-1))+AUDIO_BUFSIZE)
#define AP3_DELAY ((1UL << (DELAY_MAX_ORDER-2))+AUDIO_BUFSIZE)
#define AP4_DELAY ((1UL << (DELAY_MAX_ORDER-3))+AUDIO_BUFSIZE)

#define READMS 17

#define ADC_SCALE 2097152

static RAMB q31 outputDataL[AUDIO_BUFSIZE],
				outputDataR[AUDIO_BUFSIZE],
				mixL[AUDIO_BUFSIZE],
				mixR[AUDIO_BUFSIZE],
				lfoData[AUDIO_BUFSIZE],
				scratch[AUDIO_BUFSIZE],
				scratch2[AUDIO_BUFSIZE],
				ap1Data[AP1_DELAY], ap2Data[AP2_DELAY], ap3Data[AP3_DELAY], ap4Data[AP4_DELAY];

struct allpass *ap1, *ap2, *ap3, *ap4;

#define NUM_TAPS 4
#define NUM_DELAY_BLOCKS 512
#define DELAY_SIZE (AUDIO_BUFSIZE*NUM_DELAY_BLOCKS)
#define DELAY_MIN 256
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
		 filterLast = 0,
		 modRateLast = 0,
		 modDepthLast = 0,
		 phaserRateLast = 0,
		 phaserIntensityLast = 0,
		 apRocLast = 0,
		 apMixLast = 0;

q31 delayMix = 0, delayFeedback = 0, delayMult = 0, tapSpacing = 0, modRange = 0, apMix = 0;

static int dtime = 0, counter = 0;

float filterGain = 0;

struct lfo *pans[NUM_TAPS];
static const q28 panRates[] = {
		_F28(1.0), _F28(2.3), _F28(.4), _F28(.8)
};

static const q31 panDepths[] = {
		_F(.999), _F(.999), _F(.999), _F(.999)
};

static uint32_t tapBase[NUM_TAPS] = {0, 0, 0, 0};

static uint32_t modBottom[NUM_TAPS] = {0, 0, 0, 0};
static uint32_t modTop[NUM_TAPS] = {0, 0, 0, 0};
static q16 roc[NUM_TAPS] = {0, 0, 0, 0};

static q16 rocMin[NUM_TAPS] = { _F16(.002), _F16(.0038), _F16(.0046), _F16(.006) };
static q16 rocMax[NUM_TAPS] = { _F16(.014), _F16(.010), _F16(.018), _F16(.015) };

struct tilt *filter;

ak4558 iface;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(2, PIN_NEOPIX, SPORT1, NEO_GRB + NEO_KHZ800);

void audioHook(q31 *data)
{
	deinterleave(data, outputDataL, outputDataR);

	//process the phaser first
	triangle(phaserLFO, lfoData);

	q31 in;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		phaserout = outputDataL[i] + _mult32x32(phaserout, phaserFeedback);
		for(int j=0; j<NUM_AP; j++){
			q31 kval = phaserk[j] + lfoData[i];
			in = phaserout + _mult32x32(phaserLast[j], kval);
			phaserout = phaserLast[j] + _mult32x32(in, __builtin_bfin_negate_fr1x32(kval));
			phaserLast[j] = in;
		}
		scratch[i] = phaserout;
	}
	wetdry(outputDataL, scratch, _mult32x32(phaserDepth, delayMix));

	copy(scratch2, outputDataL);
	copy(outputDataR, outputDataL);

	zero(mixL);
	zero(mixR);

	//scale feedback mix by mult
	q31 fbMix = _mult32x32(delayFeedback, 0x7FFFFFFF - _mult32x32(0x7FFFFFFF - _F(1./NUM_TAPS), delayMult));

	//scale feedback mix to compensate for filter gain
	fbMix = _mult32x32(fbMix, 0x7FFFFFFF - (0x7FFFFFFF/2/512*abs((int)filterLast-512)));
	for(int i=0; i<NUM_TAPS; i++){
		bool doneMoving = taps[i]->roc == 0;
		if(!doneMoving) _delay_move(taps[i], scratch, AUDIO_BUFSIZE);
		else{
			taps[i]->roc = roc[i] + _mult32x32(roc[i], modRange);
			taps[i]->bottom = modBottom[i];
			taps[i]->top = modTop[i];
		}
		if(doneMoving && taps[i]->roc > 0){
			delayModulate(taps[i], scratch);
			taps[i]->roc = 0; //set roc back to 0
		}
		else if(doneMoving) delayPop(taps[i], scratch);

		//gain all taps after the main one
		if(i > 0) gain(scratch, scratch, taps[i]->coeff);

		//process the panning
		triangle(pans[i], lfoData);
		pan(scratch, lfoData, mixL, mixR);

		mix(scratch2, scratch, fbMix);
	}

	allpassModulate(ap1, scratch2, scratch);
	allpassModulate(ap2, scratch, scratch);
	allpassModulate(ap3, scratch, scratch);
	allpassModulate(ap4, scratch, scratch);

	wetdry(scratch2, scratch, apMix);

	processTilt(filter, scratch2);
	delayPush(line, scratch2);

	mix(outputDataL, mixL, delayMix);
	mix(outputDataR, mixR, delayMix);

	interleave(data, outputDataL, outputDataR);
}

void setup(){
	phaserLFO = initLFO(_F28(.7), _F(.1));

	ap1 = initAllpass(ap1Data, AP1_DELAY, 2048);
	ap2 = initAllpass(ap2Data, AP2_DELAY, 1024);
	ap3 = initAllpass(ap3Data, AP3_DELAY, 512);
	ap4 = initAllpass(ap4Data, AP4_DELAY, 256);

	filter = initTilt(2000);

	line = initDelayLine(delayBuf, DELAY_SIZE);
	for(int i=0; i<NUM_TAPS; i++){
		taps[i] = initDelayTap(line, 0);
		pans[i] = initLFO(panRates[i], panDepths[i]);
	}

	controls.begin();

	pixels.begin();
	pixels.setPixelColor(1, pixels.Color(50,0,0));

	iface.begin();

	//begin fx processor
	fx.begin();

	//set the function to be called when a buffer is ready
	fx.setHook(audioHook);
}

void updateDelayTimes(bool move = true){
	if(move){
		int timeMax = map(delayTimeLast, 0, 1023, DELAY_MIN, DELAY_SIZE);
		for(int i=0; i<NUM_TAPS; i++){
			if(i == 0) tapBase[i] = timeMax;
			else{
				q31 point = tapRateMin[i] + _mult32x32((tapRateMax[i] - tapRateMin[i]), tapSpacing);
				tapBase[i] = _mult32x32(timeMax, point);
			}
			if(taps[i]->currentOffset > tapBase[i]){
				taps[i]->bottom = tapBase[i];
				taps[i]->direction = _F(-1.0);
			}
			else if(taps[i]->currentOffset < tapBase[i]){
				taps[i]->top = tapBase[i];
				taps[i]->direction = 0x7FFFFFFF;
			}
			taps[i]->roc = min(abs((int)taps[i]->currentOffset - (int)tapBase[i]) * 10, _F16(1.0));
		}
		dtime = (float)(tapBase[0])/AUDIO_SAMPLE_RATE*1000/READMS;
	}
	//update the modulation settings
	for(int i=0; i<NUM_TAPS; i++){
		uint32_t newtop = tapBase[i] + _mult32x32(tapBase[i], modRange);
		modTop[i] = min(newtop, DELAY_SIZE - 1);

		int newBottom = tapBase[i] - _mult32x32(tapBase[i], modRange);
		modBottom[i] = max(newBottom, DELAY_MIN);
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

	//Alternate controls
	if(controls.state.adcAlt[MOD_DEPTH_KNOB] != modDepthLast){
		modDepthLast = controls.state.adcAlt[MOD_DEPTH_KNOB];
		modRange = modDepthLast * (ADC_SCALE>>4);
		updateDelayTimes(false);
	}
	if(controls.state.adcAlt[MOD_RATE_KNOB] != modRateLast){
		modRateLast = controls.state.adcAlt[MOD_RATE_KNOB];
		for(int i=0; i<NUM_TAPS; i++){
			if(modRateLast < 20) roc[i] = 0;
			else roc[i] = rocMin[i] + _mult32x32((rocMax[i] - rocMin[i]), modRateLast*ADC_SCALE);
		}
	}
	if(controls.state.adcAlt[PHASER_RATE_KNOB] != phaserRateLast){
		phaserRateLast = controls.state.adcAlt[PHASER_RATE_KNOB];
		phaserLFO->rate = (q31)phaserRateLast * 500000;
	}
	if(controls.state.adcAlt[PHASER_INTENSITY_KNOB] != phaserIntensityLast){
		phaserIntensityLast = controls.state.adcAlt[PHASER_INTENSITY_KNOB];
		phaserDepth = (q31)phaserIntensityLast * ADC_SCALE;
		phaserFeedback = (q31)phaserIntensityLast * ADC_SCALE >> 2;
	}
	if(controls.state.adcAlt[AP_ROC_KNOB] != apRocLast){
		apRocLast = controls.state.adcAlt[AP_ROC_KNOB];
	    ap4->tap->roc = apRocLast >> 2;
	    ap3->tap->roc = ap4->tap->roc * 2;
	    ap2->tap->roc = ap3->tap->roc * 2;
	    ap1->tap->roc = ap2->tap->roc * 2;
	}
	if(controls.state.adcAlt[AP_MIX_KNOB] != apMixLast){
		apMixLast = controls.state.adcAlt[AP_MIX_KNOB];
		apMix = (q31)apMixLast * ADC_SCALE;
	}

	//flash LED with delay time
	uint8_t blue = controls.state.btns.bit.alt * 50;
	if(counter%dtime==0){
		pixels.setPixelColor(0, pixels.Color(50,0,blue));
		counter = 0;
	}
	else if (counter < dtime/2)
		pixels.setPixelColor(0, pixels.Color(50,0,blue));
	else
		pixels.setPixelColor(0, pixels.Color(0,0,blue));
	counter++;

	pixels.show();
	delay(READMS);
	__asm__ volatile("IDLE;");
}
