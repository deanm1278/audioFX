/*
 * audioRingBuf.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: Dean
 */


#include "audioRingBuf.h"
#include "variant.h"

AudioRingBuf::AudioRingBuf(int32_t *buf, uint32_t size, AudioFX *fx, uint32_t addrOffset)
{
	startAddr = (uint32_t)buf;
	head = (int32_t *)startAddr;
	tail = (int32_t *)startAddr;

	//TODO: assert that end isn't past the end of l2 memory
	end = (int32_t *)(startAddr + (AUDIO_BUFSIZE << 1) * sizeof(int32_t) * size);
	count = 0;
	cap = size;
	_fx = fx;
}

void AudioRingBuf::resize(uint32_t size)
{
	//TODO: assert that end isn't past the end of l2 memory
	end = (int32_t *)(startAddr + (AUDIO_BUFSIZE << 1) * sizeof(int32_t) * size);
	cap = size;
}

void AudioRingBuf::pushInterleaved(int32_t *data)
{
	_fx->_arb.queue(head, data, sizeof(int32_t), sizeof(int32_t), AUDIO_BUFSIZE << 1);

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (int32_t *)startAddr;
}

void AudioRingBuf::push(int32_t *leftBlock, int32_t *rightBlock)
{
	_fx->interleave(head, leftBlock, rightBlock);

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (int32_t *)startAddr;
}

void AudioRingBuf::pushCore(int32_t *leftBlock, int32_t *rightBlock)
{
	int32_t *ptr = head;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*ptr++ = *leftBlock++;
		*ptr++ = *rightBlock++;
	}

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (int32_t *)startAddr;
}

void AudioRingBuf::pop(int32_t *leftBlock, int32_t *rightBlock)
{
	pop(leftBlock, rightBlock, NULL, NULL);
}

void AudioRingBuf::pop(int32_t *leftBlock, int32_t *rightBlock, void (*fn)(void), volatile bool *done)
{
	_fx->deinterleave(leftBlock, rightBlock, tail, fn, done);

	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (int32_t *)startAddr;
}

void AudioRingBuf::pop(int32_t *leftBlock, int32_t *rightBlock, void (*fn)(void))
{
	pop(leftBlock, rightBlock, fn, NULL);
}

void AudioRingBuf::pop(int32_t *leftBlock, int32_t *rightBlock, volatile bool *done)
{
	pop(leftBlock, rightBlock, NULL, done);
}

void AudioRingBuf::popCore(int32_t *leftBlock, int32_t *rightBlock){

	int32_t *ptr = tail;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*leftBlock++ = *ptr++;
		*rightBlock++ = *ptr++;
	}
	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (int32_t *)startAddr;
}

void AudioRingBuf::peek(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset){
	peek(leftBlock, rightBlock, offset, NULL);
}

void AudioRingBuf::peek(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset, volatile bool *done)
{
	int32_t *ptr;
	uint32_t distFromEnd = (end - tail) / (AUDIO_BUFSIZE << 1);

	if(offset >= distFromEnd) {
		ptr = (int32_t *)startAddr + (AUDIO_BUFSIZE << 1) * (offset - distFromEnd);
	}
	else ptr = tail + (AUDIO_BUFSIZE << 1) * offset;

	_fx->deinterleave(leftBlock, rightBlock, ptr, NULL, done);
}

void AudioRingBuf::discard( void ){
	//toss out the last sample
	tail += (AUDIO_BUFSIZE << 1);
	count--;
	if(tail == end) tail = (int32_t *)startAddr;
}

void AudioRingBuf::peekCore(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset){

	int32_t *ptr;
	uint32_t distFromEnd = (end - tail) / (AUDIO_BUFSIZE << 1);

	if(offset >= distFromEnd) {
		ptr = (int32_t *)startAddr + (AUDIO_BUFSIZE << 1) * (offset - distFromEnd);
	}
	else ptr = tail + (AUDIO_BUFSIZE << 1) * offset;

	//if(offset > distFromEnd) asm volatile("EMUEXCPT;");

	int32_t *l = leftBlock;
	int32_t *r = rightBlock;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*l++ = *ptr++;
		*r++ = *ptr++;
	}
}

void AudioRingBuf::peekHeadCore(int32_t *leftBlock, int32_t *rightBlock, uint32_t offset){

	int32_t *ptr;
	offset = offset + 1;
	uint32_t distFromStart = ((uint32_t)head - startAddr) / (AUDIO_BUFSIZE << 3);

	if(offset > distFromStart) {
		ptr = end - (AUDIO_BUFSIZE << 1) * (offset - distFromStart);
	}
	else ptr = head - (AUDIO_BUFSIZE << 1) * offset;

	//if(offset > distFromEnd) asm volatile("EMUEXCPT;");

	int32_t *l = leftBlock;
	int32_t *r = rightBlock;
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*l++ = *ptr++;
		*r++ = *ptr++;
	}
}

void AudioRingBuf::peekHeadSamples(int32_t *left, int32_t *right, uint32_t offset, uint32_t size, void (*fn)(void)){
	int32_t *ptr, *lastBlock;
	uint32_t distFromStart;

	if(head == (int32_t *)startAddr)
		lastBlock = end - (AUDIO_BUFSIZE << 1);
	else
		lastBlock = head - (AUDIO_BUFSIZE << 1);

	distFromStart = ((uint32_t)lastBlock - startAddr) >> 2;

	//samples are interleaved
	offset = (offset + size) << 1;

	if(offset > distFromStart) {
		ptr = end - (offset - distFromStart);
	}
	else ptr = lastBlock - offset;

	int32_t *end_addr = (ptr + (size << 1) );
	if( end_addr > end ){
		//hard to synchronize, for now lets take the CPU hit
		//uint32_t diff = (end_addr - end) >> 1;

		while(size > 0){
			if(ptr == end) ptr = (int32_t *)startAddr;
			*left++ = *ptr++;
			*right++ = *ptr++;
			size--;
		}
		fn();
	}
	else{
		_fx->_arb.queue(left, ptr, sizeof(int32_t), (sizeof(int32_t) << 1), size);
		_fx->_arb.queue(right, ptr + 1, sizeof(int32_t), (sizeof(int32_t) << 1), size, fn);
	}
}
