#ifndef _LIBAUDIOFX_DELAY_H_
#define _LIBAUDIOFX_DELAY_H_

struct delayTap {
	struct delayLine *parent; //pointer to the parent delay line
	q31 *dptr; //location of the current tap
	uint32_t currentOffset; //offset of dptr from the head of the parent delay line
	q16 roc; //the rate of change of the delay tap
	uint32_t top; //the max value currentOffset can take
	q31 direction; // +1 or -1
	q16 err;
};

struct delayLine{
	q31 *head; //new samples will be pushed here
	q31 *data; //the data buffer
	uint32_t size; //the size of the data buffer in words
	struct delayTap *taps; //pointer to the taps in this delay line
};

extern "C" {
extern void _delay_push(struct delayLine *line, q31 *buf, uint32_t num);
extern void _delay_modulate(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_pitch_shift_down(struct delayTap *tap, q31 *buf, uint32_t num);
};

#endif
