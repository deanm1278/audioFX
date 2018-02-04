#include "audioFX.h"
#include "audioRingBuf.h"
#include "scheduler.h"
#include "fft.h"
#include "impulse_response.h"

AudioFX fx;
Scheduler sch;
FFT fft;

#define IR_MAX 32

#define FILTER_SIZE_0 (2*AUDIO_BUFSIZE*2)
#define FILTER_SIZE_2 (4*AUDIO_BUFSIZE*2)

#define N0 4
#define N0_START 0
#define N0_SIZE AUDIO_BUFSIZE

#define N2 4
#define N2_START (FILTER_SIZE_0 * N0_SIZE * N0)
#define N2_SIZE (AUDIO_BUFSIZE * 2)

complex_q31 convData[N0*FILTER_SIZE_0];
static AudioRingBuf<complex_q31> convBuf(convData, N0*2, &fx); //each filter0 block is 2 audio_bufsize blocks

L2DATA complex_q31 convData2[N2*FILTER_SIZE_2];
static AudioRingBuf<complex_q31> convBuf2(convData2, N2*4, &fx); //each filter2 block is 4 audio_bufsize blocks

L2DATA q31 outputData[IR_MAX * (AUDIO_BUFSIZE*2)];
static AudioRingBuf<q31> outputBuf(outputData, IR_MAX, &fx);

static uint32_t zero[4] = {0, 0, 0, 0};

typedef struct convState {
	complex_q31 *dataSrc;
	q31 *dataOut;
};

convState NState, N2State, N4State, N8State, N16State, N32State;

RAMB q31 inputL[AUDIO_BUFSIZE*2], inputR[AUDIO_BUFSIZE*2];

RAMB complex_q31 input2L[AUDIO_BUFSIZE*4], input2R[AUDIO_BUFSIZE*4];

/************ SCRATCH DATA BLOCK CONVOLVE ****************/
RAMB complex_q31	scratchNL[AUDIO_BUFSIZE*2], scratchNR[AUDIO_BUFSIZE*2],
					scratchN2L[AUDIO_BUFSIZE*2], scratchN2R[AUDIO_BUFSIZE*2],
					fftBufL[AUDIO_BUFSIZE*2], fftBufR[AUDIO_BUFSIZE*2];

uint32_t numBlocks = 0;


static inline void cmadd(AudioRingBuf<complex_q31> *buf, complex_q31 *l, complex_q31 *r, complex_q31 *src, complex_q31 *IR, uint16_t num)
{
	complex_q31 *lptr = l;
	complex_q31 *rptr = r;
	complex_q31 *srcPtr = src;
	complex_q31 *IRPtr = IR;

	//TODO: use circular addressing
	for(int i=0; i<num; i++){
		lptr->re += __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->re ) - __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->im );
		lptr->im += __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->re ) + __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->im );
		lptr++; srcPtr++; IRPtr++;

		rptr->re += __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->re ) - __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->im );
		rptr->im += __builtin_bfin_mult_fr1x32x32( srcPtr->im, IRPtr->re ) + __builtin_bfin_mult_fr1x32x32( srcPtr->re, IRPtr->im );
		rptr++; srcPtr++; IRPtr++;

		if(i% (AUDIO_BUFSIZE*2) == 0)
			srcPtr = buf->nextValid(srcPtr);
	}
}


//This assumes that the input blocks are already created and FFT'd
static inline void blockConvolve(AudioRingBuf<complex_q31> *buf, complex_q31 *l, complex_q31 *r, complex_q31 *src, complex_q31 *IR, q31 *output, uint16_t num, uint16_t N)
{
	complex_q31 *srcPtr = src;
	uint32_t mask;

	//clear out scratches
	memset(l, 0, 2*N*sizeof(complex_q31));
	memset(r, 0, 2*N*sizeof(complex_q31));

	for(uint16_t i=0; i<num; i++){
		//complex multiply-add the frequency domain signals to convolve
		cmadd(buf, l, r, srcPtr, IR, 2*N);

		//go back in time for necessary buffer
		for(int k=0; k<(N/AUDIO_BUFSIZE); k++){
			srcPtr = buf->previousValid(srcPtr - 2*AUDIO_BUFSIZE);
			srcPtr = buf->previousValid(srcPtr - 2*AUDIO_BUFSIZE);
		}

		IR += i*4*N;
	}

	//ifft both blocks for time
	mask = noInterrupts();
	fft.ifft(l, 2*N);
	fft.ifft(r, 2*N);
	interrupts(mask);

	l = l + N;
	r = r + N;

	//discard first N samples and add to output
	for(uint16_t i=0; i<N; i++){
		*output++ += l++->re;
		*output++ += r++->re;
	}
}

void processN()
{
	uint32_t mask;

	//transform both channels to frequency domain
	mask = noInterrupts();
	fft.fft(inputL, fftBufL, AUDIO_BUFSIZE*2);
	fft.fft(inputR, fftBufR, AUDIO_BUFSIZE*2);
	interrupts(mask);

	//push both blocks into the buffer
	convBuf.push(fftBufL, fftBufR);
	convBuf.push(fftBufL + AUDIO_BUFSIZE, fftBufR + AUDIO_BUFSIZE);

	//convolve N0 blocks of AUDIO_BUFSIZE
	blockConvolve(&convBuf, scratchNL, scratchNR, NState.dataSrc, impulse_response + N0_START, NState.dataOut, N0, N0_SIZE);
}

void processN2(){
	uint32_t mask;

	//transform both channels to frequency domain
	mask = noInterrupts();
	fft.fft(input2L, AUDIO_BUFSIZE*4);
	fft.fft(input2R, AUDIO_BUFSIZE*4);
	interrupts(mask);

	//push 4 blocks into the buffer
	convBuf2.push(input2L, input2R);
	convBuf2.push(input2L + AUDIO_BUFSIZE, input2R + AUDIO_BUFSIZE);
	convBuf2.push(input2L + AUDIO_BUFSIZE*2, input2R + AUDIO_BUFSIZE*2);
	convBuf2.push(input2L + AUDIO_BUFSIZE*3, input2R + AUDIO_BUFSIZE*3);

	//convolve
	blockConvolve(&convBuf2, scratchN2L, scratchN2R, NState.dataSrc, impulse_response + N2_START, N2State.dataOut, N2, N2_SIZE);
}

void audioHook(q31 *data)
{
	numBlocks++;

	//get output
	q31 *outPtr = data;
	q31 *last = outputBuf.getFront();

	//copy old samples to new position
	q31 *l = inputL + AUDIO_BUFSIZE;
	q31 *r = inputR + AUDIO_BUFSIZE;

	memcpy(inputL, l, AUDIO_BUFSIZE*sizeof(q31));
	memcpy(inputR, r, AUDIO_BUFSIZE*sizeof(q31));

	q31 *ptr = last;

	//de-interleave data and set current output
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*l++ = *outPtr;
		*outPtr++ += *ptr++;
		*r++ = *outPtr;
		*outPtr++ += *ptr++;
	}
	outputBuf.discard();
	outputBuf.clear(last);

	//most recent block
	NState.dataSrc = convBuf.getBack();
	NState.dataOut = outputBuf.getFront();

	sch.addTask(processN, SCHEDULER_MAX_PRIO);

	if(numBlocks % 2 == 0){

		//shift back old data
		fx._arb.queue(input2L, input2L + (AUDIO_BUFSIZE*2), sizeof(complex_q31), sizeof(complex_q31), AUDIO_BUFSIZE*2, sizeof(complex_q31));
		fx._arb.queue(input2R, input2R + (AUDIO_BUFSIZE*2), sizeof(complex_q31), sizeof(complex_q31), AUDIO_BUFSIZE*2, sizeof(complex_q31));

		//clear space for new data
		fx._arb.queue(input2L + (AUDIO_BUFSIZE*2), zero, sizeof(complex_q31), 0, AUDIO_BUFSIZE*2, sizeof(complex_q31));
		fx._arb.queue(input2R + (AUDIO_BUFSIZE*2), zero, sizeof(complex_q31), 0, AUDIO_BUFSIZE*2, sizeof(complex_q31));

		//shift in new data. skip 8 bytes in between for real->complex
		fx._arb.queue(input2L + (AUDIO_BUFSIZE*2), inputL, sizeof(complex_q31), sizeof(q31), AUDIO_BUFSIZE*2, sizeof(q31));
		fx._arb.queue(input2R + (AUDIO_BUFSIZE*2), inputR, sizeof(complex_q31), sizeof(q31), AUDIO_BUFSIZE*2, sizeof(q31));

		N2State.dataSrc = convBuf2.getBack();
		N2State.dataOut = outputBuf.peek(N0-1);
#if 0
		sch.addTask(processN2, SCHEDULER_MAX_PRIO - 1);
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
