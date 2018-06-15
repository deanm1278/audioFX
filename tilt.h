/*
 * tilt.h
 *
 *  Created on: Jun 15, 2018
 *      Author: Dean
 *
 *  from http://www.musicdsp.org/showone.php?id=267
 *  original by Lubomir I. Ivanov
 */

#ifndef AUDIOFX_TILT_H_
#define AUDIOFX_TILT_H_

#include <Arduino.h>
#include "audioFX_config.h"
#include "math.h"

struct tilt {
	q31 a0, b1;
	q31 lp_out, hgain, lgain;
	float f0;
};

static inline void setTiltGain(struct tilt *t, float gain){
	float amp = 8.65617024533; //6./log(2.)
	float pi = 22./7.;
	float sr3 = 3*AUDIO_SAMPLE_RATE;

	// conditition:
	// gfactor is the proportional gain
	//
	int gfactor = 5;
	float g1, g2;
	if (gain > 0){
	    g1 = -gfactor*gain;
	    g2 = gain;
	}
	else {
	    g1 = -gain;
	    g2 = gfactor*gain;
	}

	//two separate gains
	t->lgain = _F(exp(g1/amp)-1);
	t->hgain = _F(exp(g2/amp)-1);

	//filter
	float omega = 2*pi*t->f0;
	float n = 1./(sr3 + omega);
	float a0 = 2*omega*n;
	float b1 = (sr3 - omega)*n;

	t->a0 = _F(a0);
	t->b1 = _F(b1);
}

static inline struct tilt *initTilt(float centerFreq){
	struct tilt *t = (struct tilt *)malloc (sizeof(struct tilt));
	t->lp_out = 0;
	t->f0 = centerFreq;
	setTiltGain(t, 0);
	return t;
}

static inline void processTilt(struct tilt *t, q31 *buf){
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		q31 in = *buf;
		t->lp_out = _mult32x32(t->a0, in) + _mult32x32(t->b1, t->lp_out);
		*buf++ = in + _mult32x32(t->lgain, t->lp_out) + _mult32x32(t->hgain, (in - t->lp_out));
	}
}

#endif /* AUDIOFX_TILT_H_ */
