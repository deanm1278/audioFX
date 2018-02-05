/**************************************
 * This example shows how to do an FIR lowpass filter
 */

#include "audioFX.h"
#include "filter.h"

//create the fx object
AudioFX fx;

//These will be the coefficients for the filter
q31 coeffs[] = {
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

//filtered blocks will be populated here
RAMB q31 outputL[AUDIO_BUFSIZE],
  outputR[AUDIO_BUFSIZE],

//these will hold the inputs from the previous block
  lastBlockL[AUDIO_BUFSIZE],
  lastBlockR[AUDIO_BUFSIZE],

  left[AUDIO_BUFSIZE],
  right[AUDIO_BUFSIZE];

void audioHook(q31 *data)
{
  DEINTERLEAVE(data, left, right);

  //do the filtering
  fir32(&fc, left, outputL, lastBlockL);
  fir32(&fc, right, outputR, lastBlockR);

  //save the input samples
  AUDIO_COPY(lastBlockL, left);
  AUDIO_COPY(lastBlockR, right);

  //put the output samples where we will be expecting them
  INTERLEAVE(data, outputL, outputR);
}

void setup() {
	fx.begin();
	fx.setHook(audioHook);
}

void loop() {
	while(1) __asm__ volatile ("IDLE;");
}

