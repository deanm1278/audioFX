/*
 * audioFX.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: Dean
 */

#include "Arduino.h"
#include "audioFX.h"
#include <math.h>

#define FS (AUDIO_SAMPLE_RATE*2) //around 48khz
#define BCLK (64 * FS)
#define WLEN 24

typedef int32_t audioBuf[AUDIO_BUFSIZE << 1];

static audioBuf buffers[3];
static audioBuf *procBuf;

void (*AudioFX::audioHook)(int32_t *);

MdmaArbiter AudioFX::_arb;

uint8_t AudioFX::tempPool[AUDIO_TEMP_POOL_SIZE];
uint8_t *AudioFX::tempPoolPtr = AudioFX::tempPool;

DMADescriptor dRead0, dRead1, dRead2, dWrite0, dWrite1, dWrite2;

static DMADescriptor *dRead[3];
static DMADescriptor *dWrite[3];

static volatile uint8_t intCount = 0;

AudioFX::AudioFX( void ) : I2S(SPORT0, PIN_BCLK, PIN_FS, PIN_AD0, PIN_BD0)
{
	audioHook = NULL;

	dRead0.CFG.reg =  (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) |
			(DMA_CFG_PSIZE_4_BYTES << 4) | (DMA_CFG_WNR_READ_FROM_MEM << 1) | DMA_CFG_ENABLE;
	dRead0.XCNT.reg = AUDIO_BUFSIZE << 1;
	dRead0.XMOD.reg = 4;
	memcpy(&dRead1, &dRead0, sizeof(DMADescriptor));
	memcpy(&dRead2, &dRead0, sizeof(DMADescriptor));

	dRead0.DSCPTR_NXT.reg = (uint32_t)&dRead1;
	dRead0.ADDRSTART.reg = (uint32_t)buffers[0];

	dRead1.DSCPTR_NXT.reg = (uint32_t)&dRead2;
	dRead1.ADDRSTART.reg = (uint32_t)buffers[1];

	dRead2.DSCPTR_NXT.reg = (uint32_t)&dRead0;
	dRead2.ADDRSTART.reg = (uint32_t)buffers[2];

	dRead[0] = &dRead0;
	dRead[1] = &dRead1;
	dRead[2] = &dRead2;

	dWrite0.CFG.reg = (DMA_CFG_INT_X_COUNT << 20) | (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) |
				(DMA_CFG_PSIZE_4_BYTES << 4) | (DMA_CFG_WNR_WRITE_TO_MEM << 1) | DMA_CFG_ENABLE;
	dWrite0.XCNT.reg = AUDIO_BUFSIZE << 1;
	dWrite0.XMOD.reg = 4;
	memcpy(&dWrite1, &dWrite0, sizeof(DMADescriptor));
	memcpy(&dWrite2, &dWrite0, sizeof(DMADescriptor));

	dWrite0.DSCPTR_NXT.reg = (uint32_t)&dWrite1;
	dWrite0.ADDRSTART.reg = (uint32_t)buffers[0];

	dWrite1.DSCPTR_NXT.reg = (uint32_t)&dWrite2;
	dWrite1.ADDRSTART.reg = (uint32_t)buffers[1];

	dWrite2.DSCPTR_NXT.reg = (uint32_t)&dWrite0;
	dWrite2.ADDRSTART.reg = (uint32_t)buffers[2];

	dWrite[0] = &dWrite0;
	dWrite[1] = &dWrite1;
	dWrite[2] = &dWrite2;
}

bool AudioFX::begin( void )
{
	//begin i2s
	I2S::begin(BCLK, FS, WLEN);

	DMA[SPORT0_A_DMA]->DSCPTR_NXT.reg = (uint32_t)dRead[1];
	DMA[SPORT0_A_DMA]->CFG.reg = (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) | (DMA_CFG_PSIZE_4_BYTES << 4) | DMA_CFG_ENABLE;

	DMA[SPORT0_B_DMA]->DSCPTR_NXT.reg = (uint32_t)dWrite[0];
	DMA[SPORT0_B_DMA]->CFG.reg = (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) | (DMA_CFG_PSIZE_4_BYTES << 4) | (DMA_CFG_WNR_WRITE_TO_MEM << 1)|  DMA_CFG_ENABLE;

	enableIRQ(31);
	setIRQPriority(31, IRQ_MAX_PRIORITY >> 1);

	return true;
}

void AudioFX::interleave(int32_t *dest, int32_t * left, int32_t *right)
{
	_arb.queue(dest, left, sizeof(int32_t) * 2, sizeof(int32_t), AUDIO_BUFSIZE, sizeof(int32_t));
	_arb.queue(dest + 1, right, sizeof(int32_t) * 2, sizeof(int32_t), AUDIO_BUFSIZE, sizeof(int32_t));
}

void AudioFX::deinterleave(int32_t * left, int32_t *right, int32_t *src, void (*cb)(void))
{
	_arb.queue(left, src, sizeof(int32_t), sizeof(int32_t) * 2, AUDIO_BUFSIZE, sizeof(int32_t));
	_arb.queue(right, src + 1, sizeof(int32_t), sizeof(int32_t) * 2, AUDIO_BUFSIZE, sizeof(int32_t), cb);
}

void AudioFX::deinterleave(int32_t * left, int32_t *right, int32_t *src)
{
	deinterleave(left, right, src, NULL);
}

uint8_t *AudioFX::tempAlloc(void *data, uint8_t size)
{
	uint8_t *ptr = tempPoolPtr;
	memcpy(tempPoolPtr, data, size);
	tempPoolPtr += size;
	return ptr;
}

AudioFX fx;

extern "C" {

int SPORT0_B_DMA_Handler (int IQR_NUMBER )
{
	DMA[SPORT0_B_DMA]->STAT.bit.IRQDONE = 1;

	procBuf = &buffers[intCount];

	intCount = (intCount + 1) % 3;

	AudioFX::tempPoolPtr = AudioFX::tempPool;

	if(AudioFX::audioHook != NULL) AudioFX::audioHook(*procBuf);

	return IQR_NUMBER;
}

};
