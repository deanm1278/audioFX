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
		{DMA[SYS_MDMA0_SRC], DMA[SYS_MDMA0_DST], 55, true, NULL},
		{DMA[SYS_MDMA1_SRC], DMA[SYS_MDMA1_DST], 57, true, NULL},
		{DMA[SYS_MDMA2_SRC], DMA[SYS_MDMA2_DST], 59, true, NULL},
};

MdmaArbiter::MdmaArbiter( void )
{

}

bool MdmaArbiter::queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod,
		uint32_t count, uint16_t elementSize, void (*cb)(void))
{
	if(cnt == MAX_JOBS){
		__asm__ volatile("EMUEXCPT;");
		return false;
	}

	//TODO: generate more descriptors based on element size
	DMADescriptor *dr = (DMADescriptor *)malloc(sizeof(DMADescriptor));
	DMADescriptor *dw = (DMADescriptor *)malloc(sizeof(DMADescriptor));
	dr->ADDRSTART.reg = (uint32_t)src;
	dr->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
	dr->CFG.bit.EN = DMA_CFG_ENABLE;
	dr->CFG.bit.WNR = DMA_CFG_WNR_READ_FROM_MEM;
	dr->XCNT.reg = count;
	dr->XMOD.reg = srcMod;
	//TODO: set addr of next descriptor

	dw->ADDRSTART.reg = (uint32_t)dst;
	dw->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
	dw->CFG.bit.EN = DMA_CFG_ENABLE;
	dw->CFG.bit.WNR = DMA_CFG_WNR_WRITE_TO_MEM;
	dw->XCNT.reg = count;
	dw->CFG.bit.INT = DMA_CFG_INT_X_COUNT;
	dw->XMOD.reg = dstMod;
	//TODO: set addr of next descriptor

	switch(elementSize){
		case 1:
			dr->CFG.bit.MSIZE = DMA_MSIZE_1_BYTES;
			dw->CFG.bit.MSIZE = DMA_MSIZE_1_BYTES;
			break;
		case 2:
			dr->CFG.bit.MSIZE = DMA_MSIZE_2_BYTES;
			dw->CFG.bit.MSIZE = DMA_MSIZE_2_BYTES;
			break;
		case 4:
			dr->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
			dw->CFG.bit.MSIZE = DMA_MSIZE_4_BYTES;
			break;
		default:
			__asm__ volatile("EMUEXCPT;");
			break;
	}
	head->cb = cb;

	head->dRead.length = 1;
	head->dRead.list = dr;

	head->dWrite.length = 1;
	head->dWrite.list = dw;

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
		ch->readChannel->DSCPTR_NXT.reg = (uint32_t)tail->dRead.list;
		ch->writeChannel->DSCPTR_NXT.reg = (uint32_t)tail->dWrite.list;

		ch->readChannel->CFG.reg = (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) | DMA_CFG_ENABLE;
		ch->writeChannel->CFG.reg = (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) | (DMA_CFG_WNR_WRITE_TO_MEM << 1) | DMA_CFG_ENABLE;

		ch->cb = tail->cb;
		ch->dReadList = tail->dRead.list;
		ch->dWriteList = tail->dWrite.list;

		setIRQPriority(ch->IRQ, IRQ_MAX_PRIORITY >> 1);
		enableIRQ(ch->IRQ);

		//check for DMA errors
		if(ch->readChannel->STAT.bit.IRQERR ||
				ch->writeChannel->STAT.bit.IRQERR){
			__asm__ volatile ("EMUEXCPT;");
		}

		if(tail == &jobBuf[MAX_JOBS - 1]) tail = jobBuf;
		else tail++;
		cnt--;
	}
}

extern "C" {

void mdma_handler( int num ){
	MdmaArbiter::channels[num].readChannel->CFG.bit.EN = DMA_CFG_DISABLE;
	MdmaArbiter::channels[num].writeChannel->CFG.bit.EN = DMA_CFG_DISABLE;

	//free descriptors
	free(MdmaArbiter::channels[num].dReadList);
	free(MdmaArbiter::channels[num].dWriteList);

	disableIRQ(MdmaArbiter::channels[num].IRQ);
	MdmaArbiter::channels[num].available = true;
	if(MdmaArbiter::channels[num].cb != NULL) MdmaArbiter::channels[num].cb();
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
