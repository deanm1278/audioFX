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
	void pushSync(T *leftBlock, T *rightBlock);
	void push(T *data);

	void pop(T *leftBlock, T *rightBlock);
	void pop(T *leftBlock, T *rightBlock, void (*fn)(void));

	void popSync(T *leftBlock, T *rightBlock);
	void popSync(T *data);

	T *peek(uint32_t offset);
	void peek(T *leftBlock, T *rightBlock, uint32_t offset=0);
	void peekSync(T *leftBlock, T *rightBlock, uint32_t offset=0);
	void peekBackSync(T *leftBlock, T *rightBlock, uint32_t offset=0);

	T *peekBack(uint32_t offset);
	void peekBack(T *left, T *right, uint32_t offset, void (*fn)(void));
	void peekBack(T *left, T *right, uint32_t offset, uint32_t size, void (*fn)(void));

	void clear( T *ptr );

	void discard( void );
	void bump( void );

	T *nextValid(T *ptr){
		if(ptr >= end) return (T *)startAddr;
		else return ptr;
	}

	T *previousValid(T *ptr){
		if(ptr < (T *)startAddr) return end - (AUDIO_BUFSIZE << 1);
		else return ptr;
	}

	bool full( void ) { return count == cap; }
	bool empty( void ) { return count == 0; }
	uint32_t getCount( void ) { return count; }
	void setCount(uint32_t c) { count  = c; }
	T *getFront( void ){ return tail; }
	T *getBack( void ){ return head; }

protected:
	uint32_t startAddr, cap;
	volatile uint32_t count;
	T *head, *tail, *end;
	AudioFX *_fx;

};

template class AudioRingBuf<q31>;
template class AudioRingBuf<complex_q31>;

#endif /* AUDIOFX_AUDIORINGBUF_H_ */
