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

#define AUDIO_COPY(dst,src) memcpy(dst, src, AUDIO_BUFSIZE * sizeof(int32_t))
#define ARRAY_COUNT_32(x) (sizeof(x)/sizeof(q31))
#define ARRAY_END_32(x) (x + ARRAY_COUNT_32(x))

class AudioFX : public I2S
{
public:
	AudioFX( void );
	bool begin( void );
	void processBuffer( void );
	void setCallback( void (*fn)(int32_t *, int32_t *) );
	void setHook( void (*fn)(int32_t *) ){ audioHook = fn; }

	void interleave(int32_t *dest, int32_t * left, int32_t *right);
	void deinterleave(int32_t * left, int32_t *right, int32_t *src, volatile bool *done);
	void deinterleave(int32_t * left, int32_t *right, int32_t *src, void (*cb)(void), volatile bool *done);
	static void (*audioHook)(int32_t *);

private:
	void (*audioCallback)(int32_t *, int32_t *);

	static Timer _tmr;
	static MdmaArbiter _arb;
};

#endif /* LIB_AUDIOFX_H_ */
