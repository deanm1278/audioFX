/*
 * audioRingBuf.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: Dean
 */


#include "audioRingBuf.h"
#include "variant.h"

static uint32_t zero[4] = {0, 0, 0, 0};

template <class T>
AudioRingBuf<T>::AudioRingBuf(T *buf, uint32_t size, uint32_t addrOffset)
{
	startAddr = (uint32_t)buf;
	head = (T *)startAddr;
	tail = (T *)startAddr;

	end = (T *)(startAddr + (AUDIO_BUFSIZE << 1) * sizeof(T) * size);
	count = 0;
	cap = size;
}

template <class T>
void AudioRingBuf<T>::resize(uint32_t size)
{
	end = (T *)(startAddr + (AUDIO_BUFSIZE << 1) * sizeof(T) * size);
	cap = size;
}

template <class T>
void AudioRingBuf<T>::push(T *data)
{
	fx._arb.queue(head, data, sizeof(T), sizeof(T), AUDIO_BUFSIZE << 1, sizeof(T));

	head += (AUDIO_BUFSIZE << 1);
	count = min(cap, count+1);
	if(head == end) head = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::push(T *leftBlock, T *rightBlock)
{
	fx._arb.queue(head, leftBlock, sizeof(T) * 2, sizeof(T), AUDIO_BUFSIZE, sizeof(T));
	fx._arb.queue(head + 1, rightBlock, sizeof(T) * 2, sizeof(T), AUDIO_BUFSIZE, sizeof(T));

	head += (AUDIO_BUFSIZE << 1);
	count = min(cap, count+1);
	if(head == end) head = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::pushSync(T *leftBlock, T *rightBlock)
{
	T *ptr = head;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*ptr++ = *leftBlock++;
		*ptr++ = *rightBlock++;
	}

	head += (AUDIO_BUFSIZE << 1);
	count = min(cap, count+1);;
	if(head == end) head = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::pop(T *leftBlock, T *rightBlock)
{
	pop(leftBlock, rightBlock, NULL);
}

template <class T>
void AudioRingBuf<T>::pop(T *leftBlock, T *rightBlock, void (*fn)(void))
{
	if(count > 0){
		fx._arb.queue(leftBlock, tail, sizeof(T), sizeof(T) * 2, AUDIO_BUFSIZE, sizeof(T));
		fx._arb.queue(rightBlock, tail + 1, sizeof(T), sizeof(T) * 2, AUDIO_BUFSIZE, sizeof(T), fn);

		tail += (AUDIO_BUFSIZE << 1);
		count--;
		if(tail == end) tail = (T *)startAddr;
	}
}

template <class T>
void AudioRingBuf<T>::popSync(T *leftBlock, T *rightBlock)
{
	if(count > 0){
		T *ptr = tail;
		for(int i=0; i<AUDIO_BUFSIZE; i++){
			*leftBlock++ = *ptr++;
			*rightBlock++ = *ptr++;
		}
		tail += (AUDIO_BUFSIZE << 1);
		count--;
		if(tail == end) tail = (T *)startAddr;
	}
}

template <class T>
void AudioRingBuf<T>::popSync(T *data)
{
	if(count > 0){
		memcpy(data, tail, (AUDIO_BUFSIZE << 1) * sizeof(T));

		tail += (AUDIO_BUFSIZE << 1);
		count--;
		if(tail == end) tail = (T *)startAddr;
	}
}

template <class T>
void AudioRingBuf<T>::peek(T *leftBlock, T *rightBlock, uint32_t offset)
{
	T *ptr;
	uint32_t distFromEnd = (end - tail) / (AUDIO_BUFSIZE << 1);

	if(offset >= distFromEnd) {
		ptr = (T *)startAddr + (AUDIO_BUFSIZE << 1) * (offset - distFromEnd);
	}
	else ptr = tail + (AUDIO_BUFSIZE << 1) * offset;

	fx._arb.queue(leftBlock, ptr, sizeof(T), sizeof(T) * 2, AUDIO_BUFSIZE, sizeof(T));
	fx._arb.queue(rightBlock, ptr + 1, sizeof(T), sizeof(T) * 2, AUDIO_BUFSIZE, sizeof(T));
}

template <class T>
void AudioRingBuf<T>::clear( T *ptr )
{
	//memset(ptr, 0, sizeof(T) * (AUDIO_BUFSIZE << 1));
	fx._arb.queue(ptr, zero, sizeof(T), 0, AUDIO_BUFSIZE << 1, sizeof(T));
}

template <class T>
void AudioRingBuf<T>::discard( void ){
	//toss out the last sample
	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (T *)startAddr;
}

template <class T>
void AudioRingBuf<T>::bump( void ){
	//toss out the last sample
	head += (AUDIO_BUFSIZE << 1);
	count = min(cap, count+1);;
	if(head == end) head = (T *)startAddr;
}

template <class T>
T *AudioRingBuf<T>::peek(uint32_t offset){
	T *ptr;
	uint32_t distFromEnd = (end - tail) / (AUDIO_BUFSIZE << 1);

	if(offset >= distFromEnd) {
		ptr = (T *)startAddr + (AUDIO_BUFSIZE << 1) * (offset - distFromEnd);
	}
	else ptr = tail + (AUDIO_BUFSIZE << 1) * offset;

	return ptr;
}

template <class T>
void AudioRingBuf<T>::peekSync(T *leftBlock, T *rightBlock, uint32_t offset){

	T *ptr = peek(offset);

	//if(offset > distFromEnd) asm volatile("EMUEXCPT;");

	T *l = leftBlock;
	T *r = rightBlock;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*l++ = *ptr++;
		*r++ = *ptr++;
	}
}

template <class T>
void AudioRingBuf<T>::peekBackSync(T *leftBlock, T *rightBlock, uint32_t offset){

	T *ptr;
	offset = offset + 1;
	uint32_t distFromStart = ((uint32_t)head - startAddr) / ((AUDIO_BUFSIZE << 1) * sizeof(T));

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
T *AudioRingBuf<T>::peekBack(uint32_t offset){
	T *ptr, *lastBlock;
	uint32_t distFromStart;

	if(head == (T *)startAddr)
		lastBlock = end - (AUDIO_BUFSIZE << 1);
	else
		lastBlock = head - (AUDIO_BUFSIZE << 1);

	distFromStart = ((uint32_t)lastBlock - startAddr) / (sizeof(T) * (AUDIO_BUFSIZE << 1) );

	if(offset > distFromStart) {
		ptr = end - (AUDIO_BUFSIZE << 1) - ( (offset - distFromStart) * (AUDIO_BUFSIZE << 1) );
	}
	else ptr = lastBlock - (offset * (AUDIO_BUFSIZE << 1) );

	return ptr;
}

template <class T>
void AudioRingBuf<T>::peekBack(T *left, T *right, uint32_t offset, void (*fn)(void)){
	T *ptr = peekBack(offset);

	fx._arb.queue(left, ptr, sizeof(T), (sizeof(T) << 1), AUDIO_BUFSIZE, sizeof(T));
	fx._arb.queue(right, ptr + 1, sizeof(T), (sizeof(T) << 1), AUDIO_BUFSIZE, sizeof(T), fn);
}

template <class T>
void AudioRingBuf<T>::peekBack(T *left, T *right, uint32_t offset, uint32_t size, void (*fn)(void)){
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

//TODO: fix this
#if 0
	T *end_addr = (ptr + (size << 1) );
	if( end_addr > end ){
#endif
		//hard to synchronize, for now lets take the CPU hit
		//uint32_t diff = (end_addr - end) >> 1;

		while(size > 0){
			if(ptr == end) ptr = (T *)startAddr;
			*left++ = *ptr++;
			*right++ = *ptr++;
			size--;
		}
		if(fn != NULL) fn();
#if 0
	}
	else{
		fx._arb.queue(left, ptr, sizeof(T), (sizeof(T) << 1), size, sizeof(T));
		fx._arb.queue(right, ptr + 1, sizeof(T), (sizeof(T) << 1), size, sizeof(T), fn);
	}
#endif
}
