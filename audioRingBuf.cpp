/*
 * audioRingBuf.cpp
 *
 *  Created on: Dec 8, 2017
 *      Author: Dean
 */


#include "audioRingBuf.h"
#include "variant.h"

__attribute__ ((section(".l2"))) int32_t ringBuf[256 * (AUDIO_BUFSIZE << 1)];

AudioRingBuf::AudioRingBuf(uint32_t size, AudioFX *fx, uint32_t addrOffset)
{
	startAddr = (uint32_t)ringBuf;
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

void AudioRingBuf::push(int32_t *leftBlock, int32_t *rightBlock)
{
	_fx->interleave(head, leftBlock, rightBlock);

	head += (AUDIO_BUFSIZE << 1);
	count++;
	if(head == end) head = (int32_t *)startAddr;
}

void AudioRingBuf::pop(int32_t *leftBlock, int32_t *rightBlock)
{
	pop(leftBlock, rightBlock, NULL);
}

void AudioRingBuf::pop(int32_t *leftBlock, int32_t *rightBlock, volatile bool *done)
{
	_fx->deinterleave(leftBlock, rightBlock, tail, NULL, done);

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

	for(int i=0; i<AUDIO_BUFSIZE; i++){
		leftBlock[i] = *ptr++;
		rightBlock[i] = *ptr++;
	}
}
