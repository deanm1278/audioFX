/*
 * filter.h
 *
 *  Created on: Dec 14, 2017
 *      Author: Dean
 */

#ifndef AUDIOFX_FILTER_H_
#define AUDIOFX_FILTER_H_

#include <Arduino.h>

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

typedef struct {
	const q31 *coeffs;
	q31 *end;
	uint16_t count;
} filter_coeffs;

extern void fir32(filter_coeffs *coeffs, q31 *input, q31 *output, q31 *lastInput);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* AUDIOFX_FILTER_H_ */
