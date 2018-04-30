#include "audioFX.h"
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "fm.h"
#include "delay.h"

#define ADC_SCALE_FACTOR 500000

/* the maximum number of samples the chorus voice will be delayed */
#define DELAY_MAX_ORDER (10)
#define MAX_DELAY ((1UL << DELAY_MAX_ORDER)+AUDIO_BUFSIZE)

q31 RAMB dataL[MAX_DELAY], dataR[MAX_DELAY];
q31 deinterleavedL[AUDIO_BUFSIZE], deinterleavedR[AUDIO_BUFSIZE];
q31 t1L[AUDIO_BUFSIZE], t1R[AUDIO_BUFSIZE];
q31 t2L[AUDIO_BUFSIZE], t2R[AUDIO_BUFSIZE];

struct delayLine lineL = { dataL, dataL, MAX_DELAY, 0 };
struct delayTap tap1L = { &lineL, dataL, 0, _F16(.01), 1023, 0x7FFFFFFF, };
struct delayTap tap2L = { &lineL, dataL, 0, _F16(.01), 512, 0x7FFFFFFF, };

struct delayLine lineR = { dataR, dataR, MAX_DELAY, 0 };
struct delayTap tap1R = { &lineR, dataR, 0, _F16(.003), 768, 0x7FFFFFFF, };
struct delayTap tap2R = { &lineR, dataR, 0, _F16(.003), 512, 0x7FFFFFFF, };

static q31 depth;

Adafruit_ADS1015 ads;
adau17x1 iface;

void audioLoop(q31 *data)
{
    q31 *ptr = data;
    for(int i=0; i<AUDIO_BUFSIZE; i++){
        deinterleavedL[i] = *ptr++;
        deinterleavedR[i] = *ptr++;
    }
    _delay_push(&lineL, deinterleavedL, AUDIO_BUFSIZE);
    _delay_push(&lineR, deinterleavedR, AUDIO_BUFSIZE);

    _delay_modulate(&tap1L, t1L, AUDIO_BUFSIZE);
    _delay_modulate(&tap1R, t1R, AUDIO_BUFSIZE);

    _delay_modulate(&tap2L, t2L, AUDIO_BUFSIZE);
    _delay_modulate(&tap2R, t2R, AUDIO_BUFSIZE);

    ptr = data;
    for(int i=0; i<AUDIO_BUFSIZE; i++){
        *ptr++ = __builtin_bfin_mult_fr1x32x32(t1L[i], depth) + __builtin_bfin_mult_fr1x32x32(t2L[i], depth) + *ptr;
        *ptr++ = __builtin_bfin_mult_fr1x32x32(t1R[i], depth) + __builtin_bfin_mult_fr1x32x32(t2R[i], depth) + *ptr;
    }

}

void setup(){

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
    tap1L.roc = adc1>>1;
    tap2L.roc = adc1;

    adc2 = ads.readADC_SingleEnded(2);
    tap1R.roc = adc2>>1;
    tap2R.roc = adc2;

}