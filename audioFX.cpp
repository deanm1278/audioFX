/*
 * audioFX.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: Dean
 */

#include "Arduino.h"
#include "audioFX.h"
#include <math.h>

//TODO: fix this once we can get a more accurate MCLK / FS sync

#define FS (AUDIO_SAMPLE_RATE*2) //around 48khz
#define BCLK (32 * FS)
#define WLEN 24

#define BCLK_PIN 10
#define FS_PIN 5
#define AD0_PIN 9
#define BD0_PIN 6

#define MCLK_PIN 2

struct audioBuf {
	int32_t data[AUDIO_BUFSIZE << 1];
};

static struct audioBuf buffers[3];
static audioBuf *inBuf, *outBuf, *procBuf;

static int32_t procLeft[AUDIO_BUFSIZE];
static int32_t procRight[AUDIO_BUFSIZE];

static volatile bool bufReady;

void (*AudioFX::audioHook)(int32_t *);

Timer AudioFX::_tmr(MCLK_PIN);
MdmaArbiter AudioFX::_arb;

AudioFX::AudioFX( void ) : I2S(SPORT0, BCLK_PIN, FS_PIN, AD0_PIN, BD0_PIN)
{
	inBuf = &buffers[0];
	outBuf = &buffers[1];
	procBuf = &buffers[2];
	bufReady = false;
	audioCallback = NULL;
	audioHook = NULL;
}

bool AudioFX::begin( void )
{
	//begin the MCLK
	_tmr.begin(FS*128);

	//begin i2s
	I2S::begin(BCLK, FS, WLEN);

	//setup DMA buffers
	DMA[SPORT0_B_DMA]->ADDRSTART.reg = (uint32_t)inBuf->data;

	//TODO: set based on wordLength
	DMA[SPORT0_B_DMA]->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
	DMA[SPORT0_B_DMA]->XCNT.reg = AUDIO_BUFSIZE << 1;
	DMA[SPORT0_B_DMA]->XMOD.reg = 4; //4 bytes

	DMA[SPORT0_B_DMA]->CFG.bit.WNR = DMA_CFG_WNR_WRITE_TO_MEM;

	DMA[SPORT0_B_DMA]->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
	DMA[SPORT0_B_DMA]->CFG.bit.PSIZE = DMA_CFG_PSIZE_4_BYTES;

	DMA[SPORT0_A_DMA]->ADDRSTART.reg = (uint32_t)outBuf->data;

	//TODO: set based on wordLength
	DMA[SPORT0_A_DMA]->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
	DMA[SPORT0_A_DMA]->XCNT.reg = AUDIO_BUFSIZE << 1;
	DMA[SPORT0_A_DMA]->XMOD.reg = 4; //4 bytes

	DMA[SPORT0_A_DMA]->CFG.bit.WNR = DMA_CFG_WNR_READ_FROM_MEM;

	DMA[SPORT0_A_DMA]->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
	DMA[SPORT0_A_DMA]->CFG.bit.PSIZE = DMA_CFG_PSIZE_4_BYTES;
	DMA[SPORT0_A_DMA]->CFG.bit.INT = DMA_CFG_INT_PERIPHERAL;

	_arb.begin();

	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;

	enableIRQ(29);
	setIRQPriority(29, IRQ_MAX_PRIORITY >> 1);

	return true;
}

void AudioFX::processBuffer( void )
{
	if(bufReady){
		if(audioCallback != NULL){
			audioCallback(procLeft, procRight);
			//re-interleave the buffers using the core now that we're done w/ our processing algo
			int32_t *d = procBuf->data;
			int32_t *l = procLeft;
			int32_t *r = procRight;
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				*d++ = *l++;
				*d++ = *r++;
			}
		}
		bufReady = false;
	}

}

void AudioFX::interleave(int32_t *dest, int32_t * left, int32_t *right)
{
	_arb.queue(dest, left, sizeof(int32_t) * 2, sizeof(int32_t));
	_arb.queue(dest + 1, right, sizeof(int32_t) * 2, sizeof(int32_t));
}

void AudioFX::deinterleave(int32_t * left, int32_t *right, int32_t *src, void (*cb)(void), volatile bool *done)
{
	_arb.queue(left, src, sizeof(int32_t), sizeof(int32_t) * 2);
	_arb.queue(right, src + 1, sizeof(int32_t), sizeof(int32_t) * 2, AUDIO_BUFSIZE, cb, done);
}

void AudioFX::deinterleave(int32_t * left, int32_t *right, int32_t *src, volatile bool *done)
{
	deinterleave(left, right, src, NULL, done);
}

void AudioFX::setCallback( void (*fn)(int32_t *, int32_t *) )
{
	audioCallback = fn;
}

extern "C" {

int SPORT0_A_DMA_Handler (int IQR_NUMBER )
{
	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_DISABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_DISABLE;

	//TODO: check for unfinished process buffer
	//if(bufReady) asm volatile("EMUEXCPT;");

	//rotate out the buffers
	struct audioBuf *oldOutBuf = outBuf;

	outBuf = procBuf;
	procBuf = inBuf;
	inBuf = oldOutBuf;

	DMA[SPORT0_B_DMA]->ADDRSTART.reg = (uint32_t)inBuf;
	DMA[SPORT0_A_DMA]->ADDRSTART.reg = (uint32_t)outBuf;

	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;

	//convert 24 bit 2s complement to int32_t
	//TODO: convert this to a zero overhead loop?
	int32_t *d = procBuf->data;
	for(int i=0; i<(AUDIO_BUFSIZE << 1); i++)
		*d++ = (*d << 8) / (1 << 8);

	bufReady = true;

	if(AudioFX::audioHook != NULL) AudioFX::audioHook(procBuf->data);

	return IQR_NUMBER;
}

};
