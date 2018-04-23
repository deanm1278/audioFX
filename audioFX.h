/*
 * audioFX.h
 *
 *  Created on: Dec 6, 2017
 *      Author: Dean
 */

#ifndef LIB_AUDIOFX_H_
#define LIB_AUDIOFX_H_

#include "I2S.h"
#include "dma.h"
#include "Timer.h"
#include "mdmaArbiter.h"
#include "audioFX_config.h"
#include "utility.h"

#define AUDIO_COPY(dst,src) memcpy(dst, src, AUDIO_BUFSIZE * sizeof(int32_t))
#define ARRAY_COUNT_32(x) (sizeof(x)/sizeof(q31))
#define ARRAY_END_32(x) ((const q31 *)x + ARRAY_COUNT_32(x))

#define AUDIO_SEC_TO_SAMPLES(x) ((uint32_t)(x * AUDIO_SAMPLE_RATE))
#define AUDIO_SEC_TO_BLOCKS(x) ((uint32_t)((x * AUDIO_SAMPLE_RATE)/AUDIO_BUFSIZE))

#define DEINTERLEAVE(x, left, right) { q31 *lPtr = left; q31 *rPtr = right; q31 *dPtr = x; for(int __dintcount=0; __dintcount<AUDIO_BUFSIZE; __dintcount++){ *lPtr++ = *dPtr++; *rPtr++ = *dPtr++; } }
#define INTERLEAVE(x, left, right) { q31 *lPtr = left; q31 *rPtr = right; q31 *dPtr = x; for(int __intcount=0; __intcount<AUDIO_BUFSIZE; __intcount++){ *dPtr++ = *lPtr++; *dPtr++ = *rPtr++; } }

class AudioFX : public I2S
{
public:
	AudioFX( void );
	bool begin( void );
	void setHook( void (*fn)(int32_t *) ){ audioHook = fn; }

	void interleave(int32_t *dest, int32_t * left, int32_t *right);
	void deinterleave(int32_t * left, int32_t *right, int32_t *src);
	void deinterleave(int32_t * left, int32_t *right, int32_t *src, void (*cb)(void));
	static void (*audioHook)(int32_t *);
	static Timer _tmr;
	static MdmaArbiter _arb;

	uint8_t *tempAlloc(void *data, uint8_t size);

	static uint8_t *tempPoolPtr;
	static uint8_t tempPool[AUDIO_TEMP_POOL_SIZE];
};

extern AudioFX fx;

#endif /* LIB_AUDIOFX_H_ */
