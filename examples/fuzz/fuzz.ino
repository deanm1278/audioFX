#include "audioFX.h"
#include "filter.h"
#include "scheduler.h"

AudioFX fx;
Scheduler sch;

//the level to saturate to
#define LEVEL 5000000
//the gate threshold
#define THRESH 1500000

static const q31 coeffs[] = {
  _F(-0.02010411882885732),
  _F(-0.05842798004352509),
  _F(-0.061178403647821976),
  _F(-0.010939393385338943),
  _F(0.05125096443534972),
  _F(0.033220867678947885),
  _F(-0.05655276971833928),
  _F(-0.08565500737264514),
  _F(0.0633795996605449),
  _F(0.310854403656636),
  _F(0.4344309124179415),
  _F(0.310854403656636),
  _F(0.0633795996605449),
  _F(-0.08565500737264514),
  _F(-0.05655276971833928),
  _F(0.033220867678947885),
  _F(0.05125096443534972),
  _F(-0.010939393385338943),
  _F(-0.061178403647821976),
  _F(-0.05842798004352509),
  _F(-0.02010411882885732),
};

filter_coeffs fc = {
    coeffs,           //the coeffecients for the filter
    ARRAY_END_32(coeffs),   //pointer to the end of the coefficients array
    ARRAY_COUNT_32(coeffs),   //number of coefficients
};

q31 lastBlockL[AUDIO_BUFSIZE], lastBlockR[AUDIO_BUFSIZE],
  currentBlockL[AUDIO_BUFSIZE], currentBlockR[AUDIO_BUFSIZE],
  outputBlockL[AUDIO_BUFSIZE], outputBlockR[AUDIO_BUFSIZE];

void audioHook(q31 *data)
{
  //saturate and separate
  q31 *l = currentBlockL;
  q31 *r = currentBlockR;
  q31 *ptr = data;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    //gain input way up
    *ptr = *ptr * 16;

    //remove noise below threshold
    if(abs(*ptr) < THRESH) *ptr = 0;
    //saturate
    else if(*ptr > 0) *ptr = LEVEL;
    else *ptr = -LEVEL;

    *l++ = *ptr++;
    *r++ = *ptr++;
  }

  //low pass filter
  fir32(&fc, currentBlockL, outputBlockL, lastBlockL);
  fir32(&fc, currentBlockR, outputBlockR, lastBlockR);

  //save last block for next go-round
  AUDIO_COPY(lastBlockL, currentBlockL);
  AUDIO_COPY(lastBlockR, currentBlockR);

  //re-interleave
  l = outputBlockL;
  r = outputBlockR;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *data++ = *l++;
    *data++ = *r++;
  }
}

void setup(){
  fx.begin();
  sch.begin();
  fx.setHook(audioHook);

  sch.addTask(loop, 0);
}

void loop(){
  while(1) __asm__ volatile ("IDLE;");
}
