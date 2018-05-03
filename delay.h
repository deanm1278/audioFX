#ifndef _LIBAUDIOFX_DELAY_H_
#define _LIBAUDIOFX_DELAY_H_

#define PITCH_SHIFT_SIZE 2048

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

struct pitchShift{
    struct delayLine *line;
    struct delayTap *t1;
    struct delayTap *t2;
};

extern "C" {

extern void _delay_push(struct delayLine *line, q31 *buf, uint32_t num);
extern void _delay_pop(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_modulate(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_pitch_shift_down(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_pitch_shift_up(struct delayTap *tap, q31 *buf, uint32_t num);

};

static inline struct delayTap *initDelayTap(struct delyLine *line, int offset)
{
    struct delayTap *tap = (struct delayTap *)malloc (sizeof(struct delayTap));
    tap->parent = line;
    tap->dptr = line->data + offset;
    tap->currentOffset = offset;
    tap->err = offset << 16;

    return tap;
}

static inline void delayRead(struct delayTap *tap, q31 *buf)
{

}

static inline struct pitchShift *initPitchShift(struct delayLine *line)
{
    struct delayTap *tap1 = (struct delayTap *)malloc (sizeof(struct delayTap));
    struct delayTap *tap2 = (struct delayTap *)malloc (sizeof(struct delayTap));
    struct pitchShift *shift = (struct pitchShift *)malloc (sizeof(struct pitchShift));

    tap1->parent = line;
    tap1->dptr = line->data;
    tap1->currentOffset = 0;
    tap1->err = 0;

    tap2->parent = line;
    tap2->dptr = line->data + PITCH_SHIFT_SIZE/2;
    tap2->currentOffset = PITCH_SHIFT_SIZE/2;
    tap2->err = _F16(PITCH_SHIFT_SIZE/2);

    shift->line = line;
    shift->t1 = tap1;
    shift->t2 = tap2;

    return shift;
}

static inline void shiftUp(struct pitchShift *ps, q31 *buf, q31 amount){
    ps->t1->roc = amount;
    ps->t2->roc = amount;

    q31 t1Buf[AUDIO_BUFSIZE];
    q31 t2Buf[AUDIO_BUFSIZE];

    _delay_pitch_shift_up(ps->t1, t1Buf, AUDIO_BUFSIZE);
    _delay_pitch_shift_up(ps->t2, t2Buf, AUDIO_BUFSIZE);

    for(int i=0; i<AUDIO_BUFSIZE; i++)
        *buf++ = t1Buf[i] + t2Buf[i];
}

static inline void shiftDown(struct pitchShift *ps, q31 *buf, q31 amount){
    ps->t1->roc = amount;
    ps->t2->roc = amount;

    q31 t1Buf[AUDIO_BUFSIZE];
    q31 t2Buf[AUDIO_BUFSIZE];

    _delay_pitch_shift_down(ps->t1, t1Buf, AUDIO_BUFSIZE);
    _delay_pitch_shift_down(ps->t2, t2Buf, AUDIO_BUFSIZE);

    for(int i=0; i<AUDIO_BUFSIZE; i++)
        *buf++ = t1Buf[i] + t2Buf[i];
}

#endif
