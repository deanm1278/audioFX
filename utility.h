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

#define _F28(x) ((int) ((x)*( (1<<28) )))
#define _F28_INTEGER_MASK (0xE0000000)
#define _F28_TO_F31(x) ((x) << 3)

#define _F16_TO_F28(x) ((x) << 12)

typedef int32_t q28;

typedef q31 __sin_q31(q31 x);
static __sin_q31* _sin_q31 = (__sin_q31*)0x0401cca4;

typedef q31 __cos_q31(q31 x);
static __cos_q31* _cos_q31 = (__cos_q31*)0x04014e40;

extern q28 _fm_pos;
#define FM_INC (_F28(1./(AUDIO_SAMPLE_RATE+1700)))

extern "C" {
extern q28 _mult_q28xq16_mod(q28 a, q16 b);
extern q28 _mult_q28xq28(q28 a, q28 b);
};

static inline int32_t q_mod(int32_t a, uint32_t mask){
    if(a >= 0) return a & ~(mask);
    else return a | mask;
}

static inline q31 sin_q28(q28 x){
	//input x is in units of pi/2 in range of 0.0 - 3.999
	q31 sin_param = _F28_TO_F31(x & ~(_F28_INTEGER_MASK));

	q31 result;
	if(x >= _F28(3.0))
		result = __builtin_bfin_negate_fr1x32(_sin_q31(__builtin_bfin_negate_fr1x32(sin_param)));
	else if(x >= _F28(2.0))
		result = _sin_q31(__builtin_bfin_negate_fr1x32(sin_param));
	else if(x >= _F28(1.0))
		result = __builtin_bfin_negate_fr1x32(_sin_q31(sin_param));
	else if(x >= _F28(0.0))
		result = _sin_q31(_F28_TO_F31(x));
	else
		__asm__ volatile("EMUEXCPT;");

	return result;
}

static inline q31 fm(q16 carrier, q16 mod, q16 imod){
	//returns units of radians. input x is in units pi*radians
	q28 x = _fm_pos;

	//get sine with x now as 2*freq*x in units of pi/2
	q28 y = _mult_q28xq16_mod(x, mod) * 4;
	q31 mod_out = sin_q28(y);
	mod_out = __builtin_bfin_mult_fr1x32x32(mod_out, _F(1.0/PI));
	q28 mod_shifted = mod_out >> 3;

	x = _mult_q28xq16_mod(x, carrier);
	x = x*4; //multiply by 2 and scale up to units of pi/2 (multiply by 2 again)

	mod_shifted = _mult_q28xq16_mod(mod_shifted, imod);

	q31 result = sin_q28(x + mod_shifted);

	return result;
}

static inline void fm_tick(){
	_fm_pos = (_fm_pos + FM_INC) & ~(_F28_INTEGER_MASK << 1);
}

#endif /* AUDIOFX_UTILITY_H_ */
