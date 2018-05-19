#include "audioFX.h"
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "delay.h"

using namespace FX;

#define ADC_SCALE_FACTOR 500000

/* the maximum number of samples the chorus voice will be delayed */
#define DELAY_MAX_ORDER (10)
#define MAX_DELAY ((1UL << DELAY_MAX_ORDER)+AUDIO_BUFSIZE)

RAMB q31 dataL[MAX_DELAY],
	deinterleavedL[AUDIO_BUFSIZE],
	deinterleavedR[AUDIO_BUFSIZE],
	scratch[AUDIO_BUFSIZE];

struct delayLine *lineL;
struct delayTap *tap1L, *tap2L, *tap1R, *tap2R;

static q31 depth = 0;

Adafruit_ADS1015 ads;
adau17x1 iface;

void audioLoop(q31 *data)
{
    deinterleave(data, deinterleavedL, deinterleavedR);
    copy(deinterleavedR, deinterleavedL);
    delayPush(lineL, deinterleavedL);

    delayModulate(tap1L, scratch);
    mix(deinterleavedL, scratch, depth);

    delayModulate(tap2L, scratch);
	mix(deinterleavedL, scratch, depth);

    delayModulate(tap1R, scratch);
    mix(deinterleavedR, scratch, depth);

    delayModulate(tap2R, scratch);
	mix(deinterleavedR, scratch, depth);

	interleave(data, deinterleavedL, deinterleavedR);
}

void setup(){
	lineL = initDelayLine(dataL, MAX_DELAY);

	tap1L = initDelayTap(lineL, _F16(.002), 1023);
	tap2L = initDelayTap(lineL, _F16(.002), 512);

	tap1R = initDelayTap(lineL, _F16(.01), 768);
	tap2R = initDelayTap(lineL, _F16(.01), 512);

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
    tap1L->roc = adc1>>1;
    tap2L->roc = adc1;

    adc2 = ads.readADC_SingleEnded(2);
    tap1R->roc = adc2>>1;
    tap2R->roc = adc2;
}