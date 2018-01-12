/*
 * audioRingBuf.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: Dean
 */


#include "audioRingBuf.h"
#include "variant.h"

template <class T>
AudioRingBuf<T>::AudioRingBuf(T *buf, uint32_t size, AudioFX *fx, uint32_t addrOffset)
{
	startAddr = (uint32_t)buf;
	head = (T *)startAddr;
	tail = (T *)startAddr;

	end = (T *)startAddr + (AUDIO_BUFSIZE << 1) * size;
	count = 0;
	cap = size;
	_fx = fx;
}

template <class T>
void AudioRingBuf<T>::resize(uint32_t size)
{
	end = (T *)startAddr + (AUDIO_BUFSIZE << 1) * size;
	cap = size;
}

template <class T>
void AudioRingBuf<T>::pushInterleaved(T *data)
{
	_fx->_arb.queue(head, data, sizeof(T), sizeof(T), AUDIO_BUFSIZE << 1 );

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::push(T *leftBlock, T *rightBlock)
{
	_fx->_arb.queue(head, leftBlock, sizeof(T) * 2, sizeof(T));
	_fx->_arb.queue(head + 1, rightBlock, sizeof(T) * 2, sizeof(T));

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::pushCore(T *leftBlock, T *rightBlock)
{
	T *ptr = head;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*ptr++ = *leftBlock++;
		*ptr++ = *rightBlock++;
	}

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::pop(T *leftBlock, T *rightBlock)
{
	pop(leftBlock, rightBlock, NULL, NULL);
}

template <class T>
void AudioRingBuf<T>::pop(T *leftBlock, T *rightBlock, void (*fn)(void), volatile bool *done)
{
	_fx->_arb.queue(leftBlock, tail, sizeof(T), sizeof(T) * 2);
	_fx->_arb.queue(rightBlock, tail + sizeof(T), sizeof(T), sizeof(T) * 2, AUDIO_BUFSIZE, fn, done);

	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::pop(T *leftBlock, T *rightBlock, void (*fn)(void))
{
	pop(leftBlock, rightBlock, fn, NULL);
}

template <class T>
void AudioRingBuf<T>::pop(T *leftBlock, T *rightBlock, volatile bool *done)
{
	pop(leftBlock, rightBlock, NULL, done);
}

template <class T>
void AudioRingBuf<T>::popCore(T *leftBlock, T *rightBlock){

	T *ptr = tail;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*leftBlock++ = *ptr++;
		*rightBlock++ = *ptr++;
	}
	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::peek(T *leftBlock, T *rightBlock, uint32_t offset){
	peek(leftBlock, rightBlock, offset, NULL);
}

template <class T>
void AudioRingBuf<T>::peek(T *leftBlock, T *rightBlock, uint32_t offset, volatile bool *done)
{
	T *ptr;
	uint32_t distFromEnd = (end - tail) / (AUDIO_BUFSIZE << 1);

	if(offset >= distFromEnd) {
		ptr = (T *)startAddr + (AUDIO_BUFSIZE << 1) * (offset - distFromEnd);
	}
	else ptr = tail + (AUDIO_BUFSIZE << 1) * offset;

	_fx->_arb.queue(leftBlock, ptr, sizeof(T), sizeof(T) * 2);
	_fx->_arb.queue(rightBlock, ptr + 1, sizeof(T), sizeof(T) * 2, AUDIO_BUFSIZE, NULL, done);
}

template <class T>
void AudioRingBuf<T>::discard( void ){
	//toss out the last sample
	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::peekCore(T *leftBlock, T *rightBlock, uint32_t offset){

	T *ptr;
	uint32_t distFromEnd = (end - tail) / (AUDIO_BUFSIZE << 1);

	if(offset >= distFromEnd) {
		ptr = (T *)startAddr + (AUDIO_BUFSIZE << 1) * (offset - distFromEnd);
	}
	else ptr = tail + (AUDIO_BUFSIZE << 1) * offset;

	//if(offset > distFromEnd) asm volatile("EMUEXCPT;");

	T *l = leftBlock;
	T *r = rightBlock;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		memcpy(l, ptr, sizeof(T)); ptr += sizeof(T); l += sizeof(T);
		memcpy(r, ptr, sizeof(T)); ptr += sizeof(T); r += sizeof(T);
	}
}

template <class T>
void AudioRingBuf<T>::peekHeadCore(T *leftBlock, T *rightBlock, uint32_t offset){

	T *ptr;
	offset = offset + 1;
	uint32_t distFromStart = ((uint32_t)head - startAddr) / (AUDIO_BUFSIZE << 1);

	if(offset > distFromStart) {
		ptr = end - (AUDIO_BUFSIZE << 1) * (offset - distFromStart);
	}
	else ptr = head - (AUDIO_BUFSIZE << 1) * offset;

	//if(offset > distFromEnd) asm volatile("EMUEXCPT;");

	T *l = leftBlock;
	T *r = rightBlock;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*l++ = *ptr++;
		*r++ = *ptr++;
	}
}

template <class T>
void AudioRingBuf<T>::peekHeadSamples(T *left, T *right, uint32_t offset, uint32_t size, void (*fn)(void)){
	T *ptr, *lastBlock;
	uint32_t distFromStart;

	if(head == (T *)startAddr)
		lastBlock = end - (AUDIO_BUFSIZE << 1);
	else
		lastBlock = head - (AUDIO_BUFSIZE << 1);

	distFromStart = ((uint32_t)lastBlock - startAddr) / sizeof(T);

	//samples are interleaved
	offset = (offset + size) << 1;

	if(offset > distFromStart) {
		ptr = end - (offset - distFromStart);
	}
	else ptr = lastBlock - offset;

	T *end_addr = (ptr + (size << 1) );
	if( end_addr > end ){
		//hard to synchronize, for now lets take the CPU hit
		//uint32_t diff = (end_addr - end) >> 1;

		while(size > 0){
			if(ptr == end) ptr = (T *)startAddr;
			*left++ = *ptr++;
			*right++ = *ptr++;
			size--;
		}
		fn();
	}
	else{
		_fx->_arb.queue(left, ptr, sizeof(T), (sizeof(T) << 1), size);
		_fx->_arb.queue(right, ptr + 1, sizeof(T), (sizeof(T) << 1), size, fn);
	}
}
