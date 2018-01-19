/*
 * MdmaArbiter.cpp
 *
 *  Created on: Dec 10, 2017
 *      Author: Dean
 */

#include "mdmaArbiter.h"
#include "audioFX.h"

volatile struct mdmaJob MdmaArbiter::jobBuf[MAX_JOBS];
volatile struct mdmaJob *MdmaArbiter::head = MdmaArbiter::jobBuf;
volatile struct mdmaJob *MdmaArbiter::tail = MdmaArbiter::jobBuf;
volatile uint8_t MdmaArbiter::cnt = 0;

volatile struct mdmaChannel MdmaArbiter::channels[NUM_MDMA_CHANNELS] = {
		{DMA[SYS_MDMA0_SRC], DMA[SYS_MDMA0_DST], 55, true, NULL, NULL},
		{DMA[SYS_MDMA1_SRC], DMA[SYS_MDMA1_DST], 57, true, NULL, NULL},
		{DMA[SYS_MDMA2_SRC], DMA[SYS_MDMA2_DST], 59, true, NULL, NULL},
};

MdmaArbiter::MdmaArbiter( void )
{

}

//TODO: variable element size
bool MdmaArbiter::begin( void ) {
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
	return true;
}

bool MdmaArbiter::queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod,
		uint32_t count, uint16_t elementSize, void (*cb)(void))
{
	if(cnt == MAX_JOBS){
		__asm__ volatile("EMUEXCPT;");
		return false;
	}

	head->destAddr = (uint32_t)dst;
	head->srcAddr = (uint32_t)src;
	head->dstMod = dstMod;
	head->srcMod = srcMod;
	head->count = count;


	switch(elementSize){
		case 1:
			head->elementSize = DMA_MSIZE_1_BYTES;
			break;
		case 2:
			head->elementSize = DMA_MSIZE_2_BYTES;
			break;
		case 4:
			head->elementSize = DMA_MSIZE_4_BYTES;
			break;
		case 8:
			head->elementSize = DMA_MSIZE_8_BYTES;
			break;
		default:
			__asm__ volatile("EMUEXCPT;");
			break;
	}
	head->cb = cb;
	//head->done = done;

	//if(done != NULL) *done = false;

	if(head == &jobBuf[MAX_JOBS - 1]) head = jobBuf;
	else head++;
	cnt++;

	runQueue();

	return true;
}

bool MdmaArbiter::queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod, uint32_t count, uint16_t elementSize)
{
	return queue(dst, src, dstMod, srcMod, count, elementSize, NULL);
}

bool MdmaArbiter::queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod)
{
	return queue(dst, src, dstMod, srcMod, AUDIO_BUFSIZE, 4, NULL);
}

bool MdmaArbiter::queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod, uint32_t count, void (*cb)(void))
{
	return queue(dst, src, dstMod, srcMod, count, 4, cb);
}

bool queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod, uint32_t count, uint16_t elementSize, void (*cb)(void))
{
	return queue(dst, src, dstMod, srcMod, count, elementSize, cb);
}

void MdmaArbiter::runQueue( void )
{

	while(cnt){
		volatile struct mdmaChannel *ch = NULL;
		for(int i=0; i<NUM_MDMA_CHANNELS; i++){
			if(channels[i].available){
				ch = &channels[i];
				break;
			}
		}

		if(ch == NULL) break;

		ch->available = false;
		ch->readChannel->CFG.bit.MSIZE = tail->elementSize;
		ch->readChannel->CFG.bit.PSIZE = tail->elementSize;
		ch->readChannel->ADDRSTART.reg = tail->srcAddr;
		ch->readChannel->XMOD.reg = tail->srcMod;
		ch->readChannel->XCNT.reg = tail->count;

		ch->writeChannel->CFG.bit.MSIZE = tail->elementSize;
		ch->writeChannel->CFG.bit.PSIZE = tail->elementSize;
		ch->writeChannel->ADDRSTART.reg = tail->destAddr;
		ch->writeChannel->XMOD.reg = tail->dstMod;
		ch->writeChannel->XCNT.reg = tail->count;

		ch->cb = tail->cb;
		ch->done = tail->done;

		ch->readChannel->CFG.bit.EN = DMA_CFG_ENABLE;
		ch->writeChannel->CFG.bit.EN = DMA_CFG_ENABLE;

		if(ch->readChannel->STAT.bit.RUN == 0 ||
				ch->writeChannel->STAT.bit.RUN == 0){
			__asm__ volatile ("EMUEXCPT;");
		}

		setIRQPriority(ch->IRQ, IRQ_MAX_PRIORITY >> 1);
		enableIRQ(ch->IRQ);

		if(tail == &jobBuf[MAX_JOBS - 1]) tail = jobBuf;
		else tail++;
		cnt--;
	}
}

extern "C" {

void mdma_handler( int num ){
	MdmaArbiter::channels[num].readChannel->CFG.bit.EN = DMA_CFG_DISABLE;
	MdmaArbiter::channels[num].writeChannel->CFG.bit.EN = DMA_CFG_DISABLE;
	disableIRQ(MdmaArbiter::channels[num].IRQ);
	MdmaArbiter::channels[num].available = true;
	if(MdmaArbiter::channels[num].cb != NULL) MdmaArbiter::channels[num].cb();
	//if(MdmaArbiter::channels[num].done != NULL) *(MdmaArbiter::channels[num].done) = true;
	MdmaArbiter::runQueue();
}

int SYS_MDMA0_DST_Handler (int IQR_NUMBER )
{
	mdma_handler(0);
	return IQR_NUMBER;
}

int SYS_MDMA1_DST_Handler (int IQR_NUMBER )
{
	mdma_handler(1);
	return IQR_NUMBER;
}

int SYS_MDMA2_DST_Handler (int IQR_NUMBER )
{
	mdma_handler(2);
	return IQR_NUMBER;
}

};
