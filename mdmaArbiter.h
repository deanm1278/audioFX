/*
 * mdmaArbiter.h
 *
 *  Created on: Dec 10, 2017
 *      Author: Dean
 */

#ifndef AUDIOFX_MDMAARBITER_H_
#define AUDIOFX_MDMAARBITER_H_

#include "Arduino.h"
#include "audioFX_config.h"

#define MAX_JOBS 16

struct mdmaJob {
	DMADescriptor *dRead;
	DMADescriptor *dWrite;
	void (*cb)(void);
};

struct mdmaChannel {
	__IO Dmagroup *readChannel;
	__IO Dmagroup *writeChannel;
	uint8_t IRQ;
	volatile bool available;
	void (*cb)(void);
	DMADescriptor *dReadList;
	DMADescriptor *dWriteList;
};

class MdmaArbiter {
public:
	MdmaArbiter( void );

	bool queue(void *dst, void *src);
	bool queue(void *dst, void *src, void (*cb)(void));
	bool queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod);
	bool queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod, uint32_t count, void (*cb)(void));
	bool queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod, uint32_t count = AUDIO_BUFSIZE, uint16_t elementSize = 4);
	bool queue(void *dst, void *src, uint32_t dstMod, uint32_t srcMod, uint32_t count, uint16_t elementSize, void (*cb)(void));

	static void runQueue( void );
	static volatile struct mdmaChannel channels[NUM_MDMA_CHANNELS];
private:
	uint8_t getFreeMdma( void );
	static volatile struct mdmaJob jobBuf[MAX_JOBS];
	static volatile struct mdmaJob *head, *tail;
	static volatile uint8_t cnt;
};



#endif /* AUDIOFX_MDMAARBITER_H_ */
