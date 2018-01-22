#include "audioFX.h"
#include "audioRingBuf.h"
#include "scheduler.h"
#include "fft.h"
#include "impulse_response.h"

AudioFX fx;
Scheduler sch;
FFT fft;

L2DATA complex_q31 convData[32 * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf<complex_q31> convBuf(convData, 32, &fx);

L2DATA q31 outputData[32 * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf<q31> outputBuf(outputData, 32, &fx);

q31 *realOutput;

RAMB q31 ifftOutput[AUDIO_BUFSIZE << 1];

RAMB complex_q31 inputL[AUDIO_BUFSIZE], inputR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR N BLOCK CONVOLVE ****************/
		scratchNL[AUDIO_BUFSIZE], scratchNR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR 2N BLOCK CONVOLVE ****************/
		scratch2NL[AUDIO_BUFSIZE], scratch2NR[AUDIO_BUFSIZE];

uint32_t numBlocks = 0;

//this will process 2 blocks of size N
void processN(){
	complex_q31 *IRStart = impulse_response;
	complex_q31 *dataSrc = convBuf.peekPtrHead(1);
	q31 *dataOut = outputBuf.getTail();

	//convolve
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		scratchNL[i].re = __builtin_bfin_mult_fr1x32x32( (*dataSrc).re, (*IRStart).re );
		scratchNL[i].im = __builtin_bfin_mult_fr1x32x32( (*dataSrc++).im, (*IRStart++).im );

		scratchNR[i].re = __builtin_bfin_mult_fr1x32x32( (*dataSrc).re, (*IRStart).re );
		scratchNR[i].im = __builtin_bfin_mult_fr1x32x32( (*dataSrc++).im, (*IRStart++).im );
	}
	//ifft left and right channels
	fft.ifft(scratchNL, scratchNL, AUDIO_BUFSIZE);
	fft.ifft(scratchNR, scratchNR, AUDIO_BUFSIZE);

	//add real portion to output ring buffer
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*dataOut++ += scratchNL[i].re;
		*dataOut++ += scratchNR[i].re;
	}
	outputBuf.bump();

	dataOut = outputBuf.nextValidPtr(dataOut);

	//convolve with the most recent data
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		scratchNL[i].re = __builtin_bfin_mult_fr1x32x32( (*dataSrc).re, (*IRStart).re);
		scratchNL[i].im = __builtin_bfin_mult_fr1x32x32( (*dataSrc++).im, (*IRStart++).im);

		scratchNR[i].re = __builtin_bfin_mult_fr1x32x32( (*dataSrc).re, (*IRStart).re);
		scratchNR[i].im = __builtin_bfin_mult_fr1x32x32( (*dataSrc++).im, (*IRStart++).im);
	}
	//ifft left and right channels
	fft.ifft(scratchNL, scratchNL, AUDIO_BUFSIZE);
	fft.ifft(scratchNR, scratchNR, AUDIO_BUFSIZE);

	dataSrc = convBuf.nextValidPtr(dataSrc);
	dataOut = outputBuf.nextValidPtr(dataOut);

	//add real portion to output ring buffer
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*dataOut++ += scratchNL[i].re;
		*dataOut++ += scratchNR[i].re;
	}
	outputBuf.bump();
}

void processN2(){

}

void processN4(){

}

void audioHook(q31 *data)
{
  realOutput = data;

  //de-interleave data and convert to complex
  complex_q31 *l = inputL;
  complex_q31 *r = inputR;

  //get output
  q31 *last = outputBuf.getTail();
  outputBuf.popCoreInterleaved(ifftOutput);
  outputBuf.clear(last);

  q31 *ptr = ifftOutput;

  for(int i=0; i<AUDIO_BUFSIZE; i++){
    (*l).re = *data;
    *data++ += *ptr++;

    (*l++).im = 0;

    (*r).re = *data;
    *data++ += *ptr++;

    (*r++).im = 0;
  }
  realOutput = data;

  //transform both channels to frequency domain
  fft.fft(inputL, inputL, AUDIO_BUFSIZE);
  fft.fft(inputR, inputR, AUDIO_BUFSIZE);

  numBlocks++;

  convBuf.push(inputL, inputR);

  if(convBuf.getCount() > 1){
	  sch.addTask(processN, SCHEDULER_MAX_PRIO);

	  if(numBlocks % 4 == 0){
		  sch.addTask(processN2, SCHEDULER_MAX_PRIO - 1);
	  }

	  if(numBlocks % 8 == 0){
		  sch.addTask(processN4, SCHEDULER_MAX_PRIO - 2);
	  }
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
