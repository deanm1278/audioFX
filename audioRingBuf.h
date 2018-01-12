/*
 * audioRingBuf.h
 *
 *  Created on: Dec 8, 2017
 *      Author: Dean
 */

#ifndef AUDIOFX_AUDIORINGBUF_H_
#define AUDIOFX_AUDIORINGBUF_H_

#include "audioFX.h"

template<class T> class AudioRingBuf {
public:
	AudioRingBuf(T *buf, uint32_t size, AudioFX *fx, uint32_t addrOffset = 0);
	void resize(uint32_t size);

	//non-blocking push and pop
	void push(T *leftBlock, T *rightBlock);
	void pushCore(T *leftBlock, T *rightBlock);

	void pushInterleaved(T *data);

	void pop(T *leftBlock, T *rightBlock);
	void pop(T *leftBlock, T *rightBlock, void (*fn)(void));
	void pop(T *leftBlock, T *rightBlock, volatile bool *done);
	void pop(T *leftBlock, T *rightBlock, void (*fn)(void), volatile bool *done);

	void popCore(T *leftBlock, T *rightBlock);

	void peek(T *leftBlock, T *rightBlock, uint32_t offset=0);
	void peek(T *leftBlock, T *rightBlock, uint32_t offset, volatile bool *done);
	void peekCore(T *leftBlock, T *rightBlock, uint32_t offset=0);
	void peekHeadCore(T *leftBlock, T *rightBlock, uint32_t offset=0);

	void peekHeadSamples(T *left, T *right, uint32_t offset, uint32_t size, void (*fn)(void));

	void discard( void );

	bool full( void ) { return count == cap; }
	bool empty( void ) { return count == 0; }
	uint32_t getCount( void ) { return count; }

protected:
	uint32_t startAddr, count, cap;
	T *head, *tail, *end;
	AudioFX *_fx;

};


#endif /* AUDIOFX_AUDIORINGBUF_H_ */
