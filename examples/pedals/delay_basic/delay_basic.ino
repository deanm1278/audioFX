#include "audioFX.h"
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "audioRingBuf.h"

#define DELAY_TIME_MAX 256
#define ADC_SCALE_FACTOR 1048576

L2DATA q31 delayData[DELAY_TIME_MAX*AUDIO_BUFSIZE*2];
AudioRingBuf<q31> buf(delayData, DELAY_TIME_MAX);

static q31 delayLeft[AUDIO_BUFSIZE], delayRight[AUDIO_BUFSIZE],
           feedbackLeft[AUDIO_BUFSIZE], feedbackRight[AUDIO_BUFSIZE];

Adafruit_ADS1015 ads;
adau17x1 iface;

volatile q31 feedback;
volatile q31 mix;
volatile uint32_t size = 0;

void audioLoop(int32_t *data)
{
    if(buf.full()){
        buf.peekSync(delayLeft, delayRight, DELAY_TIME_MAX - size);
        buf.discard();
    }

    for(int i=0; i<AUDIO_BUFSIZE; i++){

        feedbackLeft[i] = __builtin_bfin_mult_fr1x32x32(*data + delayLeft[i], feedback);
        *data++ = __builtin_bfin_mult_fr1x32x32(*data, _F(.999) - mix) + __builtin_bfin_mult_fr1x32x32(delayLeft[i], mix);


        feedbackRight[i] = __builtin_bfin_mult_fr1x32x32(*data + delayRight[i], feedback);
        *data++ = __builtin_bfin_mult_fr1x32x32(*data, _F(.999) - mix) + __builtin_bfin_mult_fr1x32x32(delayRight[i], mix);
    }

    buf.pushSync(feedbackLeft, feedbackRight);
}

void setup(){
    //Serial.begin(115200);

    ads.setGain(GAIN_TWO);
    ads.begin();

    iface.begin();

    fx.setHook(audioLoop);
    fx.begin();
}

void loop(){
    int16_t adc0, adc1, adc2;

    adc0 = ads.readADC_SingleEnded(0);
    size = constrain(map(adc0, 0, 2048, 1, 255), 1, 255);

    adc1 = ads.readADC_SingleEnded(1);
    feedback = adc1 * ADC_SCALE_FACTOR;

    adc2 = ads.readADC_SingleEnded(2);
    mix = adc2 * ADC_SCALE_FACTOR;

}
