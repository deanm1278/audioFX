/*
 * audioFX.cpp
 *
 *  Created on: Dec 6, 2017
 *      Author: Dean
 */

#include "Arduino.h"
#include "audioFX.h"
#include <math.h>

#define FS (48500 * 2)
#define BCLK (32 * FS)
#define WLEN 24

#define BCLK_PIN 10
#define FS_PIN 5
#define AD0_PIN 9
#define BD0_PIN 6

struct audioBuf {
	int32_t data[AUDIO_BUFSIZE << 1];
};

static struct audioBuf buffers[3];
static audioBuf *inBuf, *outBuf, *procBuf;

static int32_t procLeft[AUDIO_BUFSIZE];
static int32_t procRight[AUDIO_BUFSIZE];

static volatile bool bufReady;

void (*AudioFX::mdmaCallbacks[NUM_MDMA_CHANNELS])(void) = { NULL, NULL, NULL };

uint8_t AudioFX::getFreeMdma( void )
{
	for(int i=SYS_MDMA0_SRC; i<=SYS_MDMA2_SRC; i+=2){
		if (DMA[i]->STAT.bit.RUN == 0 &&
				DMA[i + 1]->STAT.bit.RUN == 0){
			DMA[i]->CFG.bit.EN = DMA_CFG_DISABLE;
			DMA[i + 1]->CFG.bit.EN = DMA_CFG_DISABLE;
			return i;
		}
	}
	return 0;
}

AudioFX::AudioFX( void ) : I2S(SPORT0, BCLK_PIN, FS_PIN, AD0_PIN, BD0_PIN)
{
	inBuf = &buffers[0];
	outBuf = &buffers[1];
	procBuf = &buffers[2];
	bufReady = false;
	audioCallback = NULL;
}

bool AudioFX::begin( void )
{
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

	//enable mdma channels
	for(int i=SYS_MDMA0_SRC; i<=SYS_MDMA2_SRC; i+=2){
		DMA[i]->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
		DMA[i]->CFG.bit.PSIZE = DMA_CFG_PSIZE_4_BYTES;
		DMA[i]->CFG.bit.WNR = DMA_CFG_WNR_READ_FROM_MEM;
		DMA[i]->XCNT.reg = AUDIO_BUFSIZE;

		DMA[i + 1]->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
		DMA[i + 1]->CFG.bit.PSIZE = DMA_CFG_PSIZE_4_BYTES;
		DMA[i + 1]->CFG.bit.WNR = DMA_CFG_WNR_WRITE_TO_MEM;
		DMA[i + 1]->CFG.bit.INT = DMA_CFG_INT_X_COUNT;
		DMA[i + 1]->XCNT.reg = AUDIO_BUFSIZE;
	}

	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;

	enableIRQ(29);

	return true;
}

void AudioFX::processBuffer( void )
{
	if(bufReady){
		if(audioCallback != NULL){
			//convert 24 bit 2s complement to int32_t
			//TODO: convert this to a zero overhead loop?
			for(int i=0; i< (AUDIO_BUFSIZE << 1); i+=2){
				int sampNum = i >> 1;
				int32_t intermediateL = (procBuf->data[i] << 8);
				int32_t intermediateR = (procBuf->data[i + 1] << 8);
				procLeft[sampNum] = intermediateL / (1 << 8);
				procRight[sampNum] = intermediateR / (1 << 8);
			}
			audioCallback(procLeft, procRight);
			//re-interleave the buffers
			interleave(procBuf->data, procLeft, procRight);
		}
		bufReady = false;
	}

}

void AudioFX::interleave(int32_t *dest, int32_t * left, int32_t *right)
{
	uint8_t chL = 0, chR = 0;
	while(chL == 0) chL = getFreeMdma(); //find a free mdma channel

	//write the left channel
	DMA[chL]->ADDRSTART.reg = (uint32_t)left;
	DMA[chL]->XMOD.reg = sizeof(int32_t);
	DMA[chL + 1]->ADDRSTART.reg = (uint32_t)dest;
	DMA[chL + 1]->XMOD.reg = sizeof(int32_t) << 1;

	DMA[chL]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[chL + 1]->CFG.bit.EN = DMA_CFG_ENABLE;

	while(chR == 0) chR = getFreeMdma(); //find another free channel

	//re-interleave the right channel
	DMA[chR]->ADDRSTART.reg = (uint32_t)right;
	DMA[chR]->XMOD.reg = sizeof(int32_t);
	DMA[chR + 1]->ADDRSTART.reg = (uint32_t)(dest + 1);
	DMA[chR + 1]->XMOD.reg = sizeof(int32_t) << 1;

	DMA[chR]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[chR + 1]->CFG.bit.EN = DMA_CFG_ENABLE;
}

void AudioFX::deinterleave(int32_t * left, int32_t *right, int32_t *src, void (*cb)(void))
{
	uint8_t chL = 0, chR = 0;
	while(chL == 0) chL = getFreeMdma(); //find a free mdma channel

	//write the left channel
	DMA[chL]->ADDRSTART.reg = (uint32_t)src;
	DMA[chL]->XMOD.reg = sizeof(int32_t) << 1;
	DMA[chL + 1]->ADDRSTART.reg = (uint32_t)left;
	DMA[chL + 1]->XMOD.reg = sizeof(int32_t);

	DMA[chL]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[chL + 1]->CFG.bit.EN = DMA_CFG_ENABLE;

	while(chR == 0) chR = getFreeMdma(); //find another free channel

	DMA[chR]->ADDRSTART.reg = (uint32_t)(src + 1);
	DMA[chR]->XMOD.reg = sizeof(int32_t) << 1;
	DMA[chR + 1]->ADDRSTART.reg = (uint32_t)right;
	DMA[chR + 1]->XMOD.reg = sizeof(int32_t);

	DMA[chR]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[chR + 1]->CFG.bit.EN = DMA_CFG_ENABLE;

	//TODO: clean this up
	if(cb != NULL) enableIRQ((chR - SYS_MDMA0_SRC) + 55);
	else disableIRQ((chR - SYS_MDMA0_SRC) + 55);

	mdmaCallbacks[(SYS_MDMA0_SRC - chR) >> 1] = cb;
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
	if(bufReady){
		asm volatile("EMUEXCPT;");
	}

	//rotate out the buffers
	struct audioBuf *oldOutBuf = outBuf;

	outBuf = procBuf;
	procBuf = inBuf;
	inBuf = oldOutBuf;

	DMA[SPORT0_B_DMA]->ADDRSTART.reg = (uint32_t)inBuf;
	DMA[SPORT0_A_DMA]->ADDRSTART.reg = (uint32_t)outBuf;

	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;

	bufReady = true;

	return IQR_NUMBER;
}

int SYS_MDMA0_DST_Handler (int IQR_NUMBER )
{
	DMA[SYS_MDMA0_DST]->CFG.bit.EN = DMA_CFG_DISABLE;
	disableIRQ(55);
	if(AudioFX::mdmaCallbacks[0] != NULL) AudioFX::mdmaCallbacks[0]();
	return IQR_NUMBER;
}

int SYS_MDMA1_DST_Handler (int IQR_NUMBER )
{
	DMA[SYS_MDMA1_DST]->CFG.bit.EN = DMA_CFG_DISABLE;
	disableIRQ(57);
	if(AudioFX::mdmaCallbacks[1] != NULL) AudioFX::mdmaCallbacks[1]();
	return IQR_NUMBER;
}

int SYS_MDMA2_DST_Handler (int IQR_NUMBER )
{
	DMA[SYS_MDMA2_DST]->CFG.bit.EN = DMA_CFG_DISABLE;
	disableIRQ(59);
	if(AudioFX::mdmaCallbacks[2] != NULL) AudioFX::mdmaCallbacks[2]();
	return IQR_NUMBER;
}

};
