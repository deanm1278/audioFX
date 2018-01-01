#include "audioFX.h"
#include "scheduler.h"
#include "audioRingBuf.h"
#include "filter.h"

AudioFX fx;
Scheduler sch;

//circular buffer for our early reflections network
#define ER_NETWORK_SIZE 32
__attribute__ ((section(".l2"))) int32_t ERData[ER_NETWORK_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf ERBuf(ERData, ER_NETWORK_SIZE, &fx);

//circular buffer to hold our delayed data from the filter network
#define FILTER_DELAY 64
__attribute__ ((section(".l2"))) int32_t filterDelayData[FILTER_DELAY * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf filterDelayBuf(filterDelayData, FILTER_DELAY, &fx);

//buffers for the comb filters
#define NUM_COMB_FILTERS 3

#define CF0_SIZE AUDIO_SEC_TO_BLOCKS(.05)
__attribute__ ((section(".l2"))) int32_t cf0Data[CF0_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf0Buf(cf0Data, CF0_SIZE, &fx);

#define CF1_SIZE AUDIO_SEC_TO_BLOCKS(.061)
__attribute__ ((section(".l2"))) int32_t cf1Data[CF1_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf1Buf(cf1Data, CF1_SIZE, &fx);

#define CF2_SIZE AUDIO_SEC_TO_BLOCKS(.078)
__attribute__ ((section(".l2"))) int32_t cf2Data[CF2_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf2Buf(cf2Data, CF2_SIZE, &fx);

static AudioRingBuf *combFilters[NUM_COMB_FILTERS] = { &cf0Buf, &cf1Buf, &cf2Buf };

//this will hold interleaved data
int32_t *processData;

typedef struct {
  uint16_t dl;
  int32_t gain;
} delayTap;

#define NUM_ER_TAPS 5
#define TAP_MIN AUDIO_SEC_TO_SAMPLES(.0043)
#define TAP_MAX AUDIO_SEC_TO_SAMPLES(.0797)

#define ER_SIZE ((TAP_MAX - TAP_MIN) + AUDIO_BUFSIZE)

static const delayTap taps[NUM_ER_TAPS] = {
    { TAP_MIN,              _F(.841) },
    { AUDIO_SEC_TO_SAMPLES(.0268),    _F(.379) },
    { AUDIO_SEC_TO_SAMPLES(.0458),    _F(.289) },
    { AUDIO_SEC_TO_SAMPLES(.0587),    _F(.245) },
    { TAP_MAX,              _F(.193) },
};

//These will be the coefficients for the filter
static const q31 coeffs[] = {
  _F(-0.02010411882885732),
  _F(-0.05842798004352509),
  _F(-0.061178403647821976),
  _F(-0.010939393385338943),
  _F(0.05125096443534972),
  _F(0.033220867678947885),
  _F(-0.05655276971833928),
  _F(-0.08565500737264514),
  _F(0.0633795996605449),
  _F(0.310854403656636),
  _F(0.4344309124179415),
  _F(0.310854403656636),
  _F(0.0633795996605449),
  _F(-0.08565500737264514),
  _F(-0.05655276971833928),
  _F(0.033220867678947885),
  _F(0.05125096443534972),
  _F(-0.010939393385338943),
  _F(-0.061178403647821976),
  _F(-0.05842798004352509),
  _F(-0.02010411882885732),
};

filter_coeffs fc = {
    coeffs,           //the coeffecients for the filter
    ARRAY_END_32(coeffs),   //pointer to the end of the coefficients array
    ARRAY_COUNT_32(coeffs),   //number of coefficients
};

__attribute__ ((section(".data2"))) int32_t ERLeft[ER_SIZE], ERRight[ER_SIZE];

__attribute__ ((section(".data2"))) int32_t filterInputLeft[AUDIO_BUFSIZE], filterInputRight[AUDIO_BUFSIZE];
__attribute__ ((section(".data2"))) int32_t filterLeft[AUDIO_BUFSIZE], filterRight[AUDIO_BUFSIZE];

int32_t dryData[AUDIO_BUFSIZE << 1];
int32_t scratchDataL[AUDIO_BUFSIZE], scratchDataR[AUDIO_BUFSIZE];

int32_t lastBlockL[AUDIO_BUFSIZE], lastBlockR[AUDIO_BUFSIZE];

void processER()
{
  //process early reflections network
  int32_t *p = processData;
  int32_t *l = filterInputLeft;
  int32_t *r = filterInputRight;

  int32_t *tpl[NUM_ER_TAPS];
  int32_t *tpr[NUM_ER_TAPS];

  for(int i=0; i<NUM_ER_TAPS; i++){
    tpl[i] = ERLeft + (TAP_MAX - taps[i].dl);
    tpr[i] = ERRight + (TAP_MAX - taps[i].dl);
  }

  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *l = 0;
    *r = 0;

    for(int j=0; j<NUM_ER_TAPS; j++){
      *l += __builtin_bfin_mult_fr1x32x32(*tpl[j]++, taps[j].gain);
      *r += __builtin_bfin_mult_fr1x32x32(*tpr[j]++, taps[j].gain);
    }

    //add to the output
    *p++ = FRACMUL(*p, .5) + *l;
    *p++ = FRACMUL(*p, .5) + *r;

    l++;
    r++;
  }

  //Process the comb filters
  for(int i=0; i<NUM_COMB_FILTERS; i++){
    if(combFilters[i]->full()){
      combFilters[i]->popCore(scratchDataL, scratchDataR);

      int32_t *il = filterInputLeft;
      int32_t *ir = filterInputRight;
      int32_t *l = scratchDataL;
      int32_t *r = scratchDataR;
      for(int j=0; j<AUDIO_BUFSIZE; j++){
        *l = *il + FRACMUL(*l, .7);
        *il++ += *l++;

        *r = *ir + FRACMUL(*r, .7);
        *ir++ += *r++;
      }

      combFilters[i]->pushCore(scratchDataL, scratchDataR);
    }
  }

  //do the filtering
  fir32(&fc, filterInputLeft, scratchDataL, lastBlockL);
  fir32(&fc, filterInputRight, scratchDataR, lastBlockR);

  filterDelayBuf.push(scratchDataL, scratchDataR);

  //save the input samples
  AUDIO_COPY(lastBlockL, filterInputLeft);
  AUDIO_COPY(lastBlockR, filterInputRight);
}

void processFilter()
{
  int32_t *p = processData;
  int32_t *l = filterLeft;
  int32_t *r = filterRight;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *p++ += *l++;
    *p++ += *r++;
  }
}

//schedule the early reflections task when data is ready
void ERFetchDone() { sch.addTask(processER, SCHEDULER_MAX_PRIO); }

//schedule the filter task when the filter data is ready
void filterFetchDone() { sch.addTask(processFilter, SCHEDULER_MAX_PRIO - 1); }

//This will run in interrupt context
void audioHook(int32_t *data)
{
  //save the pointer to the new data
  processData = data;

  //push to the early reflections ring buffer
  memcpy(dryData, data, (AUDIO_BUFSIZE << 1) * sizeof(int32_t));
  ERBuf.pushInterleaved(dryData);

  //begin fetching necessary data
  if(ERBuf.full()){
    ERBuf.peekHeadSamples(ERLeft, ERRight, TAP_MIN, ER_SIZE, ERFetchDone);
    ERBuf.discard();
  }

  if(filterDelayBuf.full())
    filterDelayBuf.pop(filterLeft, filterRight, filterFetchDone);
}

// the setup function runs once when you press reset or power the board
void setup() {
  fx.begin();
  sch.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);

  /* loop() will never exit, and just puts the processor in
   * idle state waiting for an interrupt.
   * Set this as the lowest priority task.
   */
  sch.addTask(loop, 0);
}

void loop() {
  while(1) __asm__ volatile ("IDLE;");
}

