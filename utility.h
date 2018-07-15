/*
 * utility.h
 *
 *  Created on: Feb 9, 2018
 *      Author: Dean
 */

#ifndef AUDIOFX_UTILITY_H_
#define AUDIOFX_UTILITY_H_

#include <Arduino.h>
#include "audioFX_config.h"

#define _F28_INTEGER_MASK (0xE0000000)
#define _F28_TO_F31(x) ((x) << 3)

#define _F16_TO_F28(x) ((x) << 12)

typedef q31 __sin_q31(q31 x);
static __sin_q31* _sin_q31 = (__sin_q31*)0x0401cca4;

typedef q31 __cos_q31(q31 x);
static __cos_q31* _cos_q31 = (__cos_q31*)0x04014e40;

extern "C" {
extern void _mult_q16(q16 *buf, q16 num);
extern q16 _mult_q16_single(q16 a, q16 b);
extern int _lfo_q31(int lastPos, q31 *buf, q16 rate, q31 depth);
extern int _lfo_q16(int lastPos, q16 *buf, q16 rate, q16 depth);
};

#endif /* AUDIOFX_UTILITY_H_ */
