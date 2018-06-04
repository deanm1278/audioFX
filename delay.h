#ifndef _LIBAUDIOFX_DELAY_H_
#define _LIBAUDIOFX_DELAY_H_

#define PITCH_SHIFT_SIZE 2048
#define BIQUAD_SIZE (AUDIO_BUFSIZE + 8)

using namespace FX;

struct delayTap {
	struct delayLine *parent; //pointer to the parent delay line
	q31 *dptr; //location of the current tap
	uint32_t currentOffset; //offset of dptr from the head of the parent delay line
	q16 roc; //the rate of change of the delay tap
	uint32_t top; //the max value currentOffset can take
	q31 direction; // +1 or -1
	q16 err;
	q31 coeff;
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

struct allpass {
	struct delayLine *line;
	struct delayTap *tap;
};

struct fir {
	struct delayLine *line;
	q31 *dptr;
	q31 *coeffs;
	uint32_t num;
};

struct biquad {
    struct delayLine *output;
    struct delayLine *input;
    q31 *outptr;
    q31 *inptr;
    q28 a1;
    q28 a2;
    q28 b0;
    q28 b1;
    q28 b2;
};

extern "C" {

extern void _delay_push(struct delayLine *line, q31 *buf, uint32_t num);
extern void _delay_pop(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_modulate(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_pitch_shift_down(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _delay_pitch_shift_up(struct delayTap *tap, q31 *buf, uint32_t num);
extern void _fir(struct fir *f, q31 *buf, uint32_t num);
extern void _biquad(struct biquad *b, q31 *buf, uint32_t num);
extern void _delay_move(struct delayTap *tap, uint32_t newOffset);

};

static inline struct delayLine *initDelayLine(q31 *buf, uint32_t size)
{
	struct delayLine *line = (struct delayLine *)malloc (sizeof(struct delayLine));
	line->data = buf;
	line->head = buf;
	line->size = size;

	return line;
}

static inline struct delayTap *initDelayTap(struct delayLine *line, int offset)
{
    struct delayTap *tap = (struct delayTap *)malloc (sizeof(struct delayTap));
    tap->parent = line;
    tap->dptr = line->data + line->size - offset;
    tap->currentOffset = offset;
    tap->err = offset << 16;

    return tap;
}

static inline struct delayTap *initDelayTap(struct delayLine *line, q16 roc, uint32_t top){
	struct delayTap *tap = initDelayTap(line, 0);
	tap->roc = roc;
	tap->top = top;
	tap->direction = 0x7FFFFFFF;

	return tap;
}

static inline struct pitchShift *initPitchShift(q31 *buf)
{
    struct delayTap *tap1 = (struct delayTap *)malloc (sizeof(struct delayTap));
    struct delayTap *tap2 = (struct delayTap *)malloc (sizeof(struct delayTap));
    struct pitchShift *shift = (struct pitchShift *)malloc (sizeof(struct pitchShift));
    struct delayLine *line = initDelayLine(buf, PITCH_SHIFT_SIZE);

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

static inline struct allpass *initAllpass(q31 *buf, uint32_t size)
{
	struct allpass *p = (struct allpass *)malloc (sizeof(struct allpass));
	struct delayLine *line = initDelayLine(buf, size);
	p->line = line;
	p->tap = initDelayTap(line, AUDIO_BUFSIZE);

	return p;
}

static inline struct allpass *initAllpass(q31 *buf, uint32_t size, uint32_t top)
{
	struct allpass *p = (struct allpass *)malloc (sizeof(struct allpass));
	struct delayLine *line = initDelayLine(buf, size);
	p->line = line;
	p->tap = initDelayTap(line, 0, top);
	p->tap->roc = 0;
    p->tap->dptr = line->data + line->size - AUDIO_BUFSIZE;
    p->tap->currentOffset = 0;
    p->tap->err = 0;

	return p;
}

static inline struct fir *initFIR(q31 *buf, uint32_t size, q31 *coeffs, uint32_t n)
{
	struct fir *f = (struct fir *)malloc (sizeof(struct fir));
	struct delayLine *line = initDelayLine(buf, size);
	f->line = line;
	f->coeffs = coeffs;
	f->num = n;
	f->dptr = buf;

	return f;
}

static inline struct biquad *initBiquad(q31 *buf, q28 a1, q28 a2, q28 b0, q28 b1, q28 b2)
{
    struct biquad *b = (struct biquad *)malloc (sizeof(struct biquad));
    struct delayLine *output = initDelayLine(buf, 4);
    struct delayLine *input = initDelayLine(buf+4, AUDIO_BUFSIZE+4);

    b->output = output;
    b->outptr = output->data;
    b->input = input;
    b->inptr = input->data;
    b->a1 = a1;
    b->a2 = a2;
    b->b0 = b0;
    b->b1 = b1;
    b->b2 = b2;

    return b;
}

static inline struct biquad *initBiquad(q31 *buf)
{
	return initBiquad(buf, 0, 0, 0, 0, 0);
}

static inline void setBiquadCoeffs(struct biquad *bq, q28 *coeffs)
{
	bq->a1 = *coeffs++;
	bq->a2 = *coeffs++;
	bq->b0 = *coeffs++;
	bq->b1 = *coeffs++;
	bq->b2 = *coeffs++;
}

/* clock cycles ~= AUDIO_BUFSIZE * num_taps * 5.1
 * [FIR (21 taps)] : 14092
 */
static inline void FIRProcess(struct fir *f, q31 *bufIn, q31 *bufOut)
{
	_delay_push(f->line, bufIn, AUDIO_BUFSIZE);
	_fir(f, bufOut, AUDIO_BUFSIZE);
}

static inline void biquadProcess(struct biquad *f, q31 *bufIn, q31 *bufOut)
{
    _delay_push(f->input, bufIn, AUDIO_BUFSIZE);
    _biquad(f, bufOut, AUDIO_BUFSIZE);
}


static inline void allpassProcess(struct allpass *ap, q31 *bufIn, q31 *bufOut)
{
	q31 in[AUDIO_BUFSIZE];
	for(int i=0; i<AUDIO_BUFSIZE; i++) in[i] = *bufIn++;
	_delay_pop(ap->tap, bufOut, AUDIO_BUFSIZE);
	mix(in, bufOut, _F(-.4));
	mix(bufOut, in, _F(.4));
	_delay_push(ap->line, in, AUDIO_BUFSIZE);
}

static inline void allpassModulate(struct allpass *ap, q31 *bufIn, q31 *bufOut)
{
	q31 in[AUDIO_BUFSIZE];
	for(int i=0; i<AUDIO_BUFSIZE; i++) in[i] = *bufIn++;
	_delay_modulate(ap->tap, bufOut, AUDIO_BUFSIZE);
	mix(in, bufOut, _F(-.5));
	mix(bufOut, in, _F(.5));
	_delay_push(ap->line, in, AUDIO_BUFSIZE);
}

static inline void delayPush(struct delayLine *line, q31 *buf){
	_delay_push(line, buf, AUDIO_BUFSIZE);
}

static inline void delayPop(struct delayTap *tap, q31 *buf){
	_delay_pop(tap, buf, AUDIO_BUFSIZE);
}

static inline void delayModulate(struct delayTap *tap, q31 *buf){
	_delay_modulate(tap, buf, AUDIO_BUFSIZE);
}

static inline void delaySum(struct delayTap *tap, q31 *buf)
{
	q31 tmp[AUDIO_BUFSIZE];
	_delay_pop(tap, tmp, AUDIO_BUFSIZE);
	mix(buf, tmp, tap->coeff);
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
