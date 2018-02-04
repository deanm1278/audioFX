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
	uint32_t mask;

	mask = noInterrupts();

	if(cnt == MAX_JOBS){
		__asm__ volatile("EMUEXCPT;");
		return false;
	}

	uint8_t numDesc = ceil(elementSize / 4);
	uint8_t msize;
	switch(elementSize){
		case 0:
			__asm__ volatile("EMUEXCPT;");
			break;
		case 1:
			msize = DMA_MSIZE_1_BYTES;
			break;
		case 2:
			msize = DMA_MSIZE_2_BYTES;
			break;
		default:
			msize = DMA_MSIZE_4_BYTES;
			break;
	}

	DMADescriptor *dr, *dw;

	DMADescriptor *drList = (DMADescriptor *)malloc(sizeof(DMADescriptor) * numDesc);
	DMADescriptor *dwList = (DMADescriptor *)malloc(sizeof(DMADescriptor) * numDesc);

	for(int i=0; i<numDesc; i++){
		dr = drList + i;
		dw = dwList + i;

		if(i == numDesc - 1){
			//last descriptor, stop flow and interrupt
			dr->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
			dw->CFG.bit.FLOW = DMA_CFG_FLOW_STOP;
			dw->CFG.bit.INT = DMA_CFG_INT_X_COUNT;
		}
		else{
			dr->CFG.bit.FLOW = DMA_CFG_FLOW_DSCL;
			dr->CFG.bit.NDSIZE = 4; //fetch 5 elements
			dw->CFG.bit.FLOW = DMA_CFG_FLOW_DSCL;
			dw->CFG.bit.NDSIZE = 4; //fetch 5 elements

			dr->DSCPTR_NXT.reg = (uint32_t)(dr + 1);
			dw->DSCPTR_NXT.reg = (uint32_t)(dw + 1);
		}

		dr->ADDRSTART.reg = (uint32_t)(src + i*4);

		dr->CFG.bit.EN = DMA_CFG_ENABLE;
		dr->CFG.bit.WNR = DMA_CFG_WNR_READ_FROM_MEM;
		dr->CFG.bit.MSIZE = msize;
		dr->XCNT.reg = count;
		dr->XMOD.reg = srcMod;

		dw->ADDRSTART.reg = (uint32_t)(dst + i*4);

		dw->CFG.bit.EN = DMA_CFG_ENABLE;
		dw->CFG.bit.WNR = DMA_CFG_WNR_WRITE_TO_MEM;
		dw->CFG.bit.MSIZE = msize;
		dw->XCNT.reg = count;
		dw->XMOD.reg = dstMod;
	}


	head->cb = cb;

	head->dRead = drList;

	head->dWrite = dwList;

	if(head == &jobBuf[MAX_JOBS - 1]) head = jobBuf;
	else head++;
	cnt++;

	runQueue();

	interrupts(mask);

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
		ch->readChannel->DSCPTR_NXT.reg = (uint32_t)tail->dRead;
		ch->writeChannel->DSCPTR_NXT.reg = (uint32_t)tail->dWrite;

		ch->readChannel->CFG.reg = (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) | DMA_CFG_ENABLE;
		ch->writeChannel->CFG.reg = (4UL << 16) | (DMA_CFG_FLOW_DSCL << 12) | (DMA_MSIZE_4_BYTES << 8) | (DMA_CFG_WNR_WRITE_TO_MEM << 1) | DMA_CFG_ENABLE;

		ch->cb = tail->cb;
		ch->dReadList = tail->dRead;
		ch->dWriteList = tail->dWrite;

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
