#include "audioFX.h"
#include "audioRingBuf.h"
#include "scheduler.h"
#include "fft.h"
#include "impulse_response.h"

AudioFX fx;
Scheduler sch;
FFT fft;

#define IR_MAX 64

L2DATA complex_q31 convData[IR_MAX * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf<complex_q31> convBuf(convData, IR_MAX, &fx);

L2DATA q31 outputData[IR_MAX * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf<q31> outputBuf(outputData, IR_MAX, &fx);

q31 *realOutput;

typedef struct convState {
	complex_q31 *dataSrc;
	q31 *dataOut;
};

convState NState, N2State, N4State, N8State, N16State, N32State;

RAMB complex_q31 inputL[AUDIO_BUFSIZE], inputR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR N BLOCK CONVOLVE ****************/
		scratchNL[AUDIO_BUFSIZE], scratchNR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR 2N BLOCK CONVOLVE ****************/
		scratch2NL[AUDIO_BUFSIZE], scratch2NR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR 4N BLOCK CONVOLVE ****************/
		scratch4NL[AUDIO_BUFSIZE], scratch4NR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR 8N BLOCK CONVOLVE ****************/
		scratch8NL[AUDIO_BUFSIZE], scratch8NR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR 16N BLOCK CONVOLVE ****************/
		scratch16NL[AUDIO_BUFSIZE], scratch16NR[AUDIO_BUFSIZE],

/************ SCRATCH DATA FOR 32N BLOCK CONVOLVE ****************/
		scratch32NL[AUDIO_BUFSIZE], scratch32NR[AUDIO_BUFSIZE];

uint32_t numBlocks = 0;

/* multiply srcPtr (freq domain) against IRPtr (freq domain) for AUDIO_BUFSIZE samples and place
 * the time domain output in l and r
 */
static inline void convolve(complex_q31 *l, complex_q31 *r, complex_q31 *src, complex_q31 *IR)
{
	//TODO: this is wrong. use overlap-save method
	complex_q31 *lptr = l;
	complex_q31 *rptr = r;
	complex_q31 *srcPtr = src;
	complex_q31 *IRPtr = IR;

	for(int i=0; i<AUDIO_BUFSIZE; i++){
		lptr->re = __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->re ) - __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->im );
		lptr->im = __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->re ) + __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->im );
		lptr++; srcPtr++; IRPtr++;

		rptr->re = __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->re ) - __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->im );
		rptr->im = __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->re ) + __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->im );
		rptr++; srcPtr++; IRPtr++;
	}

	uint32_t mask = noInterrupts();
	//ifft left and right channels
	//TODO: find out why it doesn't like being interrupted
	fft.ifft(l, l, AUDIO_BUFSIZE);
	fft.ifft(r, r, AUDIO_BUFSIZE);
	interrupts(mask);
}

//this will process 2 blocks of size N
void processN(){
	for(int j=0; j<2; j++){
		NState.dataOut = outputBuf.nextValidPtr(NState.dataOut);
		NState.dataSrc = convBuf.nextValidPtr(NState.dataSrc);

		complex_q31 *IRStart = impulse_response + j * (AUDIO_BUFSIZE << 1);

		convolve(scratchNL, scratchNR, NState.dataSrc, IRStart);

		//add real portion to output ring buffer
		for(int i=0; i<AUDIO_BUFSIZE; i++){
			*NState.dataOut++ += scratchNL[i].re;
			*NState.dataOut++ += scratchNR[i].re;
		}
	}
}

void processN2(){

	for(int k=0; k<2; k++){
		complex_q31 *src = N2State.dataSrc;

		for(int j=0; j<2; j++){
			N2State.dataOut = outputBuf.nextValidPtr(N2State.dataOut);
			src = convBuf.nextValidPtr(src);

			complex_q31 *IRStart = impulse_response + ((k*2) + j + 2) * (AUDIO_BUFSIZE << 1);

			convolve(scratch2NL, scratch2NR, src, IRStart);

			//add real portion to output ring buffer
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				*N2State.dataOut++ += scratch2NL[i].re;
				*N2State.dataOut++ += scratch2NR[i].re;
			}
			src += (AUDIO_BUFSIZE << 1);
		}
	}
}

void processN4(){
	for(int k=0; k<2; k++){
		complex_q31 *src = N2State.dataSrc;

		for(int j=0; j<4; j++){
			N4State.dataOut = outputBuf.nextValidPtr(N4State.dataOut);
			src = convBuf.nextValidPtr(src);

			complex_q31 *IRStart = impulse_response + ( (k*4) + j + 6) * (AUDIO_BUFSIZE << 1);

			convolve(scratch4NL, scratch4NR, src, IRStart);

			//add real portion to output ring buffer
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				*N4State.dataOut++ += scratch4NL[i].re;
				*N4State.dataOut++ += scratch4NR[i].re;
			}
			src += (AUDIO_BUFSIZE << 1);
		}
	}
}

void processN8(){
	for(int k=0; k<2; k++){
		complex_q31 *src = N8State.dataSrc;

		for(int j=0; j<8; j++){
			N8State.dataOut = outputBuf.nextValidPtr(N8State.dataOut);
			src = convBuf.nextValidPtr(src);

			complex_q31 *IRStart = impulse_response + ( (k*8) + j + 6) * (AUDIO_BUFSIZE << 1);

			convolve(scratch8NL, scratch8NR, src, IRStart);

			//add real portion to output ring buffer
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				*N8State.dataOut++ += scratch8NL[i].re;
				*N8State.dataOut++ += scratch8NR[i].re;
			}
			src += (AUDIO_BUFSIZE << 1);
		}
	}
}

void processN16(){
	for(int k=0; k<2; k++){
		complex_q31 *src = N16State.dataSrc;

		for(int j=0; j<16; j++){
			N8State.dataOut = outputBuf.nextValidPtr(N16State.dataOut);
			src = convBuf.nextValidPtr(src);

			complex_q31 *IRStart = impulse_response + ( (k*16) + j + 30) * (AUDIO_BUFSIZE << 1);

			convolve(scratch16NL, scratch16NR, src, IRStart);

			//add real portion to output ring buffer
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				*N16State.dataOut++ += scratch16NL[i].re;
				*N16State.dataOut++ += scratch16NR[i].re;
			}
			src += (AUDIO_BUFSIZE << 1);
		}
	}
}

void processN32(){
	for(int k=0; k<1; k++){
		complex_q31 *src = N32State.dataSrc;

		for(int j=0; j<32; j++){
			N32State.dataOut = outputBuf.nextValidPtr(N32State.dataOut);
			src = convBuf.nextValidPtr(src);

			complex_q31 *IRStart = impulse_response + ( (k*32) + j + 62) * (AUDIO_BUFSIZE << 1);

			convolve(scratch32NL, scratch32NR, src, IRStart);

			//add real portion to output ring buffer
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				*N32State.dataOut++ += scratch32NL[i].re;
				*N32State.dataOut++ += scratch32NR[i].re;
			}
			src += (AUDIO_BUFSIZE << 1);
		}
	}
}

void audioHook(q31 *data)
{
  realOutput = data;

  //de-interleave data and convert to complex
  complex_q31 *l = inputL;
  complex_q31 *r = inputR;

  //get output
  q31 *outPtr = data;
  q31 *last = outputBuf.getTail();

  q31 *ptr = last;

  for(int i=0; i<AUDIO_BUFSIZE; i++){
    (*l).re = *outPtr;
    *outPtr++ = *ptr++;
    (*l++).im = 0;

    (*r).re = *outPtr;
    *outPtr++ = *ptr++;
    (*r++).im = 0;
  }
  outputBuf.discard();
  outputBuf.clear(last);
  realOutput = data;

  //transform both channels to frequency domain
  fft.fft(inputL, inputL, AUDIO_BUFSIZE);
  fft.fft(inputR, inputR, AUDIO_BUFSIZE);

  numBlocks++;

  convBuf.pushCore(inputL, inputR);

  if(convBuf.getCount() > 1){
	//output buffer count is meaningless in this application
	outputBuf.setCount(INT32_MAX);

#if 0
	NState.dataSrc = convBuf.peekPtrHead(0);
	NState.dataOut = outputBuf.peekPtr(0);
	sch.addTask(processN, SCHEDULER_MAX_PRIO);
#endif

#if 0
	if(numBlocks % 2 == 0){
	  N2State.dataSrc = convBuf.peekPtrHead(2);
	  N2State.dataOut = outputBuf.peekPtr(2);
	  sch.addTask(processN2, SCHEDULER_MAX_PRIO - 1);
	}
#endif

#if 0
	  if(numBlocks % 4 == 0){
		  N4State.dataSrc = convBuf.peekPtrHead(4);
		  N4State.dataOut = outputBuf.peekPtr(4);
		  sch.addTask(processN4, SCHEDULER_MAX_PRIO - 2);
	  }
#endif

#if 1
	  if(numBlocks % 8 == 0){
		  N8State.dataSrc = convBuf.peekPtrHead(8);
		  N8State.dataOut = outputBuf.peekPtr(8);
		  sch.addTask(processN8, SCHEDULER_MAX_PRIO - 3);
	  }
#endif

#if 0
	  if(numBlocks % 16 == 0){
		  N16State.dataSrc = convBuf.peekPtrHead(16);
		  N16State.dataOut = outputBuf.peekPtr(16);
		  sch.addTask(processN16, SCHEDULER_MAX_PRIO - 4);
	  }
#endif

#if 0
	  if(numBlocks % 32 == 0){
		  N16State.dataSrc = convBuf.peekPtrHead(32);
		  N16State.dataOut = outputBuf.peekPtr(32);
		  sch.addTask(processN16, SCHEDULER_MAX_PRIO - 5);
	  }
#endif
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