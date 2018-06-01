// A simple phaser pedal with rate, depth, and feedback controls

#include "audioFX.h"
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "lfo.h"

using namespace FX;

#define ADC_SCALE_FACTOR 1000000

//the number of allpass filters in the phaser
#define NUM_AP 6

static q31 depth, feedback, out = 0;

RAMB q31 deinterleavedL[AUDIO_BUFSIZE],
  scratch[AUDIO_BUFSIZE],
  lfoData[AUDIO_BUFSIZE];

Adafruit_ADS1015 ads;
adau17x1 iface;

//this will be a simple triangle LFO
struct lfo *wave;

q31 last[NUM_AP];
q31 k[NUM_AP] = {
    _F(.78), _F(.64), _F(.7),
    _F(.89), _F(.74), _F(.86)
};

void audioLoop(q31 *data)
{
    deinterleave(data, deinterleavedL, scratch);

    triangle(wave, lfoData);

    q31 in;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		out = deinterleavedL[i] + _mult32x32(out, feedback);
		for(int j=0; j<NUM_AP; j++){
			q31 kval = k[j] + lfoData[i];
			in = out + _mult32x32(last[j], kval);
			out = last[j] + _mult32x32(in, __builtin_bfin_negate_fr1x32(kval));
			last[j] = in;
		}
		scratch[i] = out;
	}
	mix(deinterleavedL, scratch, depth);

	interleave(data, deinterleavedL, deinterleavedL);
}

void setup(){
  //initialize the LFO with .1 depth. The rate will get set by potentiometer
  wave = initLFO(_F28(.7), _F(.1));

  ads.setGain(GAIN_TWO);
  ads.begin();

    iface.begin();

    fx.setHook(audioLoop);
    fx.begin();
}

void loop(){
    int16_t adc0, adc1, adc2;

    adc0 = ads.readADC_SingleEnded(0);
    depth = adc0 * ADC_SCALE_FACTOR;

    adc1 = ads.readADC_SingleEnded(1);
    wave->rate = adc1 * ADC_SCALE_FACTOR >> 1;

    adc2 = ads.readADC_SingleEnded(2);
    feedback = adc2 * ADC_SCALE_FACTOR;
}
