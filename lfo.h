/*
 * lfo.h
 *
 *  Created on: May 20, 2018
 *      Author: Dean
 */

#ifndef AUDIOFX_LFO_H_
#define AUDIOFX_LFO_H_

#include <Arduino.h>
#include "audioFX_config.h"

struct lfo {
	q28 rate;
	q31 last;
	q31 depth;
	q31 dir;
};

static inline struct lfo *initLFO(q28 rate, q31 depth)
{
	struct lfo *l = (struct lfo *)malloc (sizeof(struct lfo));
	l->rate = rate;
	l->depth = depth;
	l->last = 0;
	l->dir = 0x7FFFFFFF;
	return l;
}

static inline void triangle(struct lfo *l, q31 *out){
	q31 inc = _mult32x32(l->rate, _F(1./AUDIO_SAMPLE_RATE)) << 5;
	inc = _mult32x32(inc, l->dir);
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		__asm__ volatile("%0 = %1 + %2 (S)" : "=d"(l->last) : "d"(l->last), "d"(inc));
		if(l->last == 0x7FFFFFFF) l->dir = 0x80000000;
		else if(l->last == 0x80000000) l->dir = 0x7FFFFFFF;
		*out++ = _mult32x32(l->last, l->depth);
	}
}



#endif /* AUDIOFX_LFO_H_ */
