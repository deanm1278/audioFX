/*
 * audioRingBuf.h
 *
 *  Created on: Dec 8, 2017
 *      Author: Dean
 */

#ifndef AUDIOFX_AUDIORINGBUF_H_
#define AUDIOFX_AUDIORINGBUF_H_

#include "audioFX.h"

class AudioRingBuf {
public:
	AudioRingBuf(int32_t *buf, uint32_t size, AudioFX *fx, uint32_t addrOffset = 0);
	void resize(uint32_t size);

	//non-blocking push and pop
	void push(int32_t *leftBlock, int32_t *rightBlock);
	void pushCore(int32_t *leftBlock, int32_t *rightBlock);

	void pushInterleaved(int32_t *data);

	void pop(int32_t *leftBlock, int32_t *rightBlock);
	void pop(int32_t *leftBlock, int32_t *rightBlock, void (*fn)(void));
	void pop(int32_t *leftBlock, int32_t *rightBlock, volatile bool *done);
	void pop(int32_t *leftBlock, int32_t *rightBlock, void (*fn)(void), volatile bool *done);

	void popCore(int32_t *leftBlock, int32_t *rightBlock);

	void peek(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset=0);
	void peek(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset, volatile bool *done);
	void peekCore(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset=0);
	void peekHeadCore(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset=0);

	void peekHeadSamples(int32_t *left, int32_t *right, uint32_t offset, uint32_t size, void (*fn)(void));

	void discard( void );

	bool full( void ) { return count == cap; }
	bool empty( void ) { return count == 0; }
	uint32_t getCount( void ) { return count; }

protected:
	uint32_t startAddr, count, cap;
	int32_t *head, *tail, *end;
	AudioFX *_fx;

};


#endif /* AUDIOFX_AUDIORINGBUF_H_ */
