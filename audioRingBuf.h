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

	T *peekPtr(uint32_t offset);
	T *peekPtrHead(uint32_t offset);

	//non-blocking push and pop
	void push(T *leftBlock, T *rightBlock);
	void pushCore(T *leftBlock, T *rightBlock);

	void pushInterleaved(T *data);

	void pop(T *leftBlock, T *rightBlock);
	void pop(T *leftBlock, T *rightBlock, void (*fn)(void));

	void popCore(T *leftBlock, T *rightBlock);
	void popCoreInterleaved(T *data);

	void peek(T *leftBlock, T *rightBlock, uint32_t offset=0);
	void peekCore(T *leftBlock, T *rightBlock, uint32_t offset=0);
	void peekHeadCore(T *leftBlock, T *rightBlock, uint32_t offset=0);

	void peekHead(T *left, T *right, uint32_t offset, void (*fn)(void));
	void peekHeadSamples(T *left, T *right, uint32_t offset, uint32_t size, void (*fn)(void));

	void clear( T *ptr );

	void discard( void );
	void bump( void );

	T *nextValidPtr(T *ptr){
		if(ptr >= end) return (T *)startAddr;
		else return ptr;
	}

	bool full( void ) { return count == cap; }
	bool empty( void ) { return count == 0; }
	uint32_t getCount( void ) { return count; }
	void setCount(uint32_t c) { count  = c; }
	T *getTail( void ){ return tail; }
	T *getHead( void ){ return head; }

protected:
	uint32_t startAddr, cap;
	volatile uint32_t count;
	T *head, *tail, *end;
	AudioFX *_fx;

};

template class AudioRingBuf<q31>;
template class AudioRingBuf<complex_q31>;

#endif /* AUDIOFX_AUDIORINGBUF_H_ */
