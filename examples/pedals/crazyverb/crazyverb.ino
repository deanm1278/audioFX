#include "audioFX.h"
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "delay.h"

#define ADC_SCALE_FACTOR 1000000

//****** PITCH SHIFT **********//
q31 L2DATA shiftBuf[PITCH_SHIFT_SIZE];
struct pitchShift *shift;

//******* FIR FILTER **********//
#define NUM_COEFFS 21
q31 firCoeffs[NUM_COEFFS] = {
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
RAMB q31 firbuf[AUDIO_BUFSIZE+NUM_COEFFS];

struct fir *filter;

//******* ALLPASS FILTERS AND DELAYS *********//
#define ALLPASS_SIZE1 256
#define ALLPASS_SIZE2 258
#define ALLPASS_SIZE3 270
#define ALLPASS_SIZE4 285
#define ALLPASS_SIZE5 300
#define ALLPASS_SIZE6 310
#define ALLPASS_SIZE7 312
#define ALLPASS_SIZE8 313

q31 L2DATA apDelay1[ALLPASS_SIZE1], apDelay2[ALLPASS_SIZE2],
     apDelay3[ALLPASS_SIZE3], apDelay4[ALLPASS_SIZE4],
     apDelay5[ALLPASS_SIZE5], apDelay6[ALLPASS_SIZE6],
     apDelay7[ALLPASS_SIZE7], apDelay8[ALLPASS_SIZE8];

#define DELAY_SIZE 4096
L2DATA q31 dl1[DELAY_SIZE], dl2[DELAY_SIZE], dl3[DELAY_SIZE], dl4[8192];

struct allpass *ap1, *ap2, *ap3, *ap4, *ap5, *ap6, *ap7, *ap8;

struct delayLine *delay1, *delay2, *delay3, *delay4;
struct delayTap *t1, *t2, *t3, *t4;

//******* SOME BUFFERS ****************//
q31 RAMB deinterleavedL[AUDIO_BUFSIZE],
     scratch[AUDIO_BUFSIZE],
     output[AUDIO_BUFSIZE],
     outL[AUDIO_BUFSIZE],
     outR[AUDIO_BUFSIZE];

//******* SOME PARAMETERS *************//
static q31 depth = 0, roc = 0, amt = 0;

Adafruit_ADS1015 ads;
adau17x1 iface;

void audioLoop(q31 *data)
{
  //separate left and right
    deinterleave(data, deinterleavedL, output);

    //pitch shift the input
    delayPush(shift->line, deinterleavedL);
    shiftUp(shift, output, roc);

    //mix the shifted signal with the original
    mix(deinterleavedL, output, amt);

    zero(outL);
    zero(outR);

    //filter the feedback signal
    FIRProcess(filter, scratch, scratch);

    //feed back into reverberator
    sum(scratch, deinterleavedL);
    allpassProcess(ap1, scratch, output);
    allpassProcess(ap2, output, output);
    delayPush(delay1, output);

    delayPop(t1, scratch);
    splitSum(scratch, outL, outR, _F(.2), _F(.4));
    gain(scratch, scratch, depth);
    sum(scratch, deinterleavedL);

    allpassProcess(ap3, scratch, output);
    allpassProcess(ap4, output, output);
    delayPush(delay2, output);

    delayPop(t2, scratch);
    splitSum(scratch, outL, outR, _F(.5), _F(.3));
    gain(scratch, scratch, depth);
    sum(scratch, deinterleavedL);

    allpassProcess(ap5, scratch, output);
    allpassProcess(ap6, output, output);
    delayPush(delay3, output);

    delayPop(t3, scratch);
    splitSum(scratch, outL, outR, _F(.3), _F(.4));
    gain(scratch, scratch, depth);
    sum(scratch, deinterleavedL);

    allpassProcess(ap7, scratch, output);
    allpassProcess(ap8, output, output);
    delayPush(delay4, output);

    delayModulate(t4, scratch); //t4 is a modulating tap
    splitSum(scratch, outL, outR, _F(.3), _F(.3));
    gain(scratch, scratch, depth);

    //interleave left and right outputs
    interleave(data, outL, outR);
}

void setup(){
  Serial.begin(115200);

    ads.setGain(GAIN_TWO);
    ads.begin();

    iface.begin();

    shift = initPitchShift(shiftBuf);

    ap1 = initAllpass(apDelay1, ALLPASS_SIZE1);
    ap2 = initAllpass(apDelay2, ALLPASS_SIZE2);
    ap3 = initAllpass(apDelay3, ALLPASS_SIZE3);
    ap4 = initAllpass(apDelay4, ALLPASS_SIZE4);
    ap5 = initAllpass(apDelay5, ALLPASS_SIZE5);
    ap6 = initAllpass(apDelay6, ALLPASS_SIZE6);
    ap7 = initAllpass(apDelay7, ALLPASS_SIZE7);
    ap8 = initAllpass(apDelay8, ALLPASS_SIZE8);

    delay1 = initDelayLine(dl1, DELAY_SIZE);
    t1 = initDelayTap(delay1, 2048);

    delay2 = initDelayLine(dl2, DELAY_SIZE);
    t2 = initDelayTap(delay2, 3000);

    delay3 = initDelayLine(dl3, DELAY_SIZE);
    t3 = initDelayTap(delay3, 1024);

    delay4 = initDelayLine(dl4, 8192);
    t4 = initDelayTap(delay4, _F16(.002), 8000); //this is a modulating tap

    filter = initFIR(firbuf, AUDIO_BUFSIZE+NUM_COEFFS, firCoeffs, NUM_COEFFS);

    fx.setHook(audioLoop);
    fx.begin();

}

void loop(){
    int16_t adc0, adc1, adc2;

    adc0 = ads.readADC_SingleEnded(0);
    depth = adc0 * ADC_SCALE_FACTOR;

    adc1 = ads.readADC_SingleEnded(1);
    roc = adc1 * 35;

    adc2 = ads.readADC_SingleEnded(2);
    amt = adc2 * ADC_SCALE_FACTOR;
}
