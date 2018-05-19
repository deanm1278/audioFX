/*
 * A cascaded lowpass biquad filter pedal
 */
#include "adau17x1.h"
#include "Adafruit_ADS1015.h"
#include "audioFX.h"
#include "delay.h"
#include "filter_coeffs.h"

using namespace FX;

Adafruit_ADS1015 ads;
adau17x1 iface;

//create pointers to the two filters
RAMB q31 bqData[BIQUAD_SIZE], bq2Data[BIQUAD_SIZE];
struct biquad *bq, *bq2;

RAMB q31 left[AUDIO_BUFSIZE], right[AUDIO_BUFSIZE];

//the filter parameters
int Q, cutoff;

//this will be called when a buffer is ready to be filled
void audioHook(q31 *data)
{
  deinterleave(data, left, right);

  //process both biquad filters
  biquadProcess(bq, left, left);
  biquadProcess(bq2, left, left);

  //limit to 24 bits because we're using a 24 bit DAC
  limit24(left);

  //downmix to mono for the output
  interleave(data, left, left);
}

void setup(){
  bq = initBiquad(bqData);
  bq2 = initBiquad(bq2Data);

  ads.setGain(GAIN_TWO);
  ads.begin();

  iface.begin();

  //begin fx processor
  fx.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);
}

void loop(){
  int16_t adc0, adc1, adc2;
  bool paramsChanged = false;
  
  //read cutoff potentiometer and constrain
  adc0 = ads.readADC_SingleEnded(0);
  adc0 = constrain(map(adc0, 0, 2048, 0, 255), 0, 255);
  if(adc0 != (uint16_t)cutoff){
    cutoff = adc0;
    paramsChanged = true;
  }
  
  //read Q potentiometer and constrain
  adc1 = ads.readADC_SingleEnded(1);
  adc1 = constrain(map(adc1, 0, 2048, 0, 15), 0, 15);
  if(adc1 != (uint16_t)Q){
    Q = adc1;
    paramsChanged = true;
  }
  
  adc2 = ads.readADC_SingleEnded(2);

  //if either the Q factor or the cutoff frequency have changed we need to grab new coefficients
  if(paramsChanged){
    setBiquadCoeffs(bq, lp_coeffs[Q][cutoff]);
    setBiquadCoeffs(bq2, lp_coeffs[Q][cutoff]);
  }
}
