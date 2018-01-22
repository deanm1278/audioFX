#include "audioFX.h"
#include "audioRingBuf.h"
#include "scheduler.h"
#include "fft.h"

AudioFX fx;
Scheduler sch;
FFT fft;

#define IR_MAX 64
L2DATA complex_q31 convData[IR_MAX * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf<complex_q31> convBuf(convData, IR_MAX, &fx);

q31 *realOutput;

RAMB complex_q31 inputL[AUDIO_BUFSIZE], inputR[AUDIO_BUFSIZE],
      outputL[AUDIO_BUFSIZE], outputR[AUDIO_BUFSIZE];

void processDelay(){
  //ifft both channels
  fft.ifft(outputL, AUDIO_BUFSIZE);
  fft.ifft(outputR, AUDIO_BUFSIZE);

  //add delayed signal to the input
  complex_q31 *lin = outputL;
  complex_q31 *rin = outputR;

  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *realOutput++ += (*lin++).re;
    *realOutput++ += (*rin++).re;
  }
}

//add medium priority task to process delay
void delayDataReady(){ sch.addTask(processDelay, 10); }

void audioHook(q31 *data)
{
  realOutput = data;

  //de-interleave data and convert to complex
  complex_q31 *l = inputL;
  complex_q31 *r = inputR;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    (*l).re = *data++;
    (*l++).im = 0;

    (*r).re = *data++;
    (*r++).im = 0;
  }

  //transform both channels to frequency domain
  fft.fft(inputL, inputL, AUDIO_BUFSIZE);
  fft.fft(inputR, inputR, AUDIO_BUFSIZE);

  //save to delay buffer
  convBuf.push(inputL, inputR);

  //if the buffer is full, pop out delayed signal
  if(convBuf.full()){
    convBuf.pop(outputL, outputR);
    delayDataReady();
  }
}

void setup() {
  fx.begin();
  sch.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);

  sch.addTask(loop, 0);
}

void loop() {
  while(1) __asm__ volatile ("IDLE;");
}
