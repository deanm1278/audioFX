/*
 * filter.c
 *
 *  Created on: Dec 14, 2017
 *      Author: Dean
 */


#include "filter.h"
#include "audioFX_config.h"
#include <math.h>

void fir32(filter_coeffs *coeffs, q31 *input, q31 *output, q31 *lastInput)
{
	q31* inptr = input;
	q31 *end = output + AUDIO_BUFSIZE;
	q31 *cptr, *dptr;
	int ix = 0;
	int subix;
	int offset = 0;

	while(output != end)
	{
		subix = 0;
		cptr = coeffs->coeffs;
		while(cptr != coeffs->end){
			if(subix == 0) *output = __builtin_bfin_mult_fr1x32x32(*inptr++, *cptr);
			else{
				offset = ix - subix;
				if(offset < 0) dptr = lastInput + AUDIO_BUFSIZE + offset;

				else dptr = input + offset;

				*output += __builtin_bfin_mult_fr1x32x32(*dptr, *cptr);
			}
			subix++;
			cptr++;
		}
		ix++;
		output++;
	}
}
