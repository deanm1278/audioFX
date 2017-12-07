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
	int32_t data[AUDIO_BUFSIZE];
};

static struct audioBuf buffers[3];
static audioBuf *inBuf, *outBuf, *procBuf;

static volatile bool bufReady;

AudioFX::AudioFX( void ) : I2S(SPORT0, BCLK_PIN, FS_PIN, AD0_PIN, BD0_PIN)
{
	inBuf = &buffers[0];
	outBuf = &buffers[1];
	procBuf = &buffers[2];
	bufReady = false;
}

bool AudioFX::begin( void )
{
	//begin i2s
	I2S::begin(BCLK, FS, WLEN);

	//setup DMA buffers
	DMA[SPORT0_B_DMA]->ADDRSTART.reg = (uint32_t)inBuf->data;

	//TODO: set based on wordLength
	DMA[SPORT0_B_DMA]->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
	DMA[SPORT0_B_DMA]->XCNT.reg = AUDIO_BUFSIZE;
	DMA[SPORT0_B_DMA]->XMOD.reg = 4; //4 bytes

	DMA[SPORT0_B_DMA]->CFG.bit.WNR = DMA_CFG_WNR_WRITE_TO_MEM;

	DMA[SPORT0_B_DMA]->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
	DMA[SPORT0_B_DMA]->CFG.bit.PSIZE = DMA_CFG_PSIZE_4_BYTES;

	DMA[SPORT0_A_DMA]->ADDRSTART.reg = (uint32_t)outBuf->data;

	//TODO: set based on wordLength
	DMA[SPORT0_A_DMA]->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
	DMA[SPORT0_A_DMA]->XCNT.reg = AUDIO_BUFSIZE;
	DMA[SPORT0_A_DMA]->XMOD.reg = 4; //4 bytes

	DMA[SPORT0_A_DMA]->CFG.bit.WNR = DMA_CFG_WNR_READ_FROM_MEM;

	DMA[SPORT0_A_DMA]->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
	DMA[SPORT0_A_DMA]->CFG.bit.PSIZE = DMA_CFG_PSIZE_4_BYTES;
	DMA[SPORT0_A_DMA]->CFG.bit.INT = DMA_CFG_INT_PERIPHERAL;

	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_ENABLE;

	SSI->SEC0_SCTL29.bit.IEN = 1; //enable
	SSI->SEC0_SCTL29.bit.SEN = 1; //enable

	return true;
}

void AudioFX::processBuffer( void )
{
	if(bufReady){
		if(audioCallback != NULL){
			//convert 24 bit 2s complement to int32_t
			for(int i=0; i<AUDIO_BUFSIZE; i++){
				int32_t intermediate = (procBuf->data[i] << 8);
				procBuf->data[i] = intermediate / (1 << 8);
			}
			audioCallback(procBuf->data);
		}
		bufReady = false;
	}

}

void AudioFX::setCallback( void (*fn)(int32_t *) )
{
	audioCallback = fn;
}

extern "C" {

int SPORT0_A_DMA_Handler (int IQR_NUMBER )
{
	DMA[SPORT0_A_DMA]->CFG.bit.EN = DMA_CFG_DISABLE;
	DMA[SPORT0_B_DMA]->CFG.bit.EN = DMA_CFG_DISABLE;

	//TODO: check for unfinished process buffer

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

};
