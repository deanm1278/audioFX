#include "audioFX.h"
#include "pedalControls.h"
#include "delay.h"
#include "lfo.h"
#include "ak4558.h"

using namespace FX;

#define MIX_KNOB 0
#define FEEDBACK_KNOB 1
#define TIME_KNOB 2
#define MULT_KNOB 3

#define ADC_SCALE 2097152

static RAMB q31 outputDataL[AUDIO_BUFSIZE],
        outputDataR[AUDIO_BUFSIZE],
        mixL[AUDIO_BUFSIZE],
        mixR[AUDIO_BUFSIZE],
        lfoData[AUDIO_BUFSIZE],
        scratch[AUDIO_BUFSIZE],
        scratch2[AUDIO_BUFSIZE];

#define NUM_TAPS 3
#define DELAY_SIZE (AUDIO_BUFSIZE*256)
static L2DATA q31 delayBuf[DELAY_SIZE];
struct delayLine *line;
struct delayTap *taps[NUM_TAPS];

static const q31 tapRateMuls[] = {
    0x7FFFFFFF, _F(.4), _F(.27)
};

uint16_t delayMixLast = 0,
     delayFeedbackLast = 0,
     delayTimeLast = 0,
     multLast = 0;

q31 delayMix = 0, delayFeedback = 0;

struct lfo *pans[NUM_TAPS];
static const q28 panRates[] = {
    _F28(.7), _F28(1.2), _F28(.4)
};

static const q31 panDepths[] = {
    _F(.8), _F(.5), _F(.999)
};

ak4558 iface;

void audioHook(q31 *data)
{
  deinterleave(data, outputDataL, outputDataR);
  copy(outputDataR, outputDataL);
  zero(mixL);
  zero(mixR);

  copy(scratch2, outputDataL);

  for(int i=0; i<NUM_TAPS; i++){
    delayPop(taps[i], scratch);
    if(i > 0) gain(scratch, scratch, taps[i]->coeff);

    triangle(pans[i], lfoData);
    pan(scratch, lfoData, mixL, mixR);

    mix(scratch2, scratch, delayFeedback);
  }
  delayPush(line, scratch2);

  mix(outputDataL, mixL, delayMix);
  mix(outputDataR, mixR, delayMix);

  interleave(data, outputDataL, outputDataR);
}

void setup(){
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
    int timeMax = (255 - (delayTimeLast>>2)) * AUDIO_BUFSIZE;
    for(int i=0; i<NUM_TAPS; i++)
      _delay_move(taps[i], _mult32x32(timeMax, tapRateMuls[i]));
  }
  if(controls.state.adcPrimary[MULT_KNOB] != multLast){
    multLast = controls.state.adcPrimary[MULT_KNOB];
    int interval = 1023 / (NUM_TAPS - 1);
    int depthMul = 0x7FFFFFFF / interval;
    for(int i=1; i<NUM_TAPS; i++){
      int depth = multLast - interval * (i - 1);
      if(depth <= 0) taps[i]->coeff = 0;
      else if(depth >= interval) taps[i]->coeff  = 0x7FFFFFFF;
      else taps[i]->coeff  = depthMul * depth;
    }
  }
  delay(17);
  __asm__ volatile("IDLE;");
}

