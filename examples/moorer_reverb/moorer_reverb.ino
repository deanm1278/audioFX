/* This is an implementation of Moorers reverb algorithm.
 * https://christianfloisand.wordpress.com/2012/10/18/algorithmic-reverbs-the-moorer-design/
 *
 */

#include "audioFX.h"
#include "scheduler.h"
#include "audioRingBuf.h"
#include "filter.h"

AudioFX fx;
Scheduler sch;

/*************************************************************
          Early Reflections
*************************************************************/

//circular buffer for our early reflections network
#define ER_NETWORK_SIZE 32
L2DATA q31 ERData[ER_NETWORK_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf ERBuf(ERData, ER_NETWORK_SIZE, &fx);

typedef struct {
  uint16_t dl;
  q31 gain;
} delayTap;

#define NUM_ER_TAPS 18
#define TAP_MIN AUDIO_SEC_TO_SAMPLES(.0043)
#define TAP_MAX AUDIO_SEC_TO_SAMPLES(.0797)

#define ER_SIZE ((TAP_MAX - TAP_MIN) + AUDIO_BUFSIZE)
RAMB q31 ERLeft[ER_SIZE], ERRight[ER_SIZE];

static const delayTap taps[NUM_ER_TAPS] = {
    { TAP_MIN,              		  _F(.841) },
    { AUDIO_SEC_TO_SAMPLES(.0268),    _F(.379) },
    { AUDIO_SEC_TO_SAMPLES(.0458),    _F(.289) },
    { AUDIO_SEC_TO_SAMPLES(.0587),    _F(.193) },
    { AUDIO_SEC_TO_SAMPLES(.0707),    _F(.180) },
    { AUDIO_SEC_TO_SAMPLES(.0741),    _F(.142) },
    { AUDIO_SEC_TO_SAMPLES(.0215),    _F(.504) },
    { AUDIO_SEC_TO_SAMPLES(.0270),    _F(.380) },
    { AUDIO_SEC_TO_SAMPLES(.0485),    _F(.272) },
    { AUDIO_SEC_TO_SAMPLES(.0595),    _F(.217) },
    { AUDIO_SEC_TO_SAMPLES(.0708),    _F(.181) },
    { AUDIO_SEC_TO_SAMPLES(.0753),    _F(.167) },
    { AUDIO_SEC_TO_SAMPLES(.0225),    _F(.491) },
    { AUDIO_SEC_TO_SAMPLES(.0298),    _F(.346) },
    { AUDIO_SEC_TO_SAMPLES(.0572),    _F(.192) },
    { AUDIO_SEC_TO_SAMPLES(.0612),    _F(.181) },
    { AUDIO_SEC_TO_SAMPLES(.0726),    _F(.176) },
    { TAP_MAX,           			  _F(.134) },
};

/*************************************************************
*************************************************************/

//circular buffer to hold our delayed data from the filter network
#define FILTER_DELAY 64
L2DATA q31 filterDelayData[FILTER_DELAY * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf filterDelayBuf(filterDelayData, FILTER_DELAY, &fx);

RAMB q31 filterInputLeft[AUDIO_BUFSIZE], filterInputRight[AUDIO_BUFSIZE];
RAMB q31 filterLeft[AUDIO_BUFSIZE], filterRight[AUDIO_BUFSIZE];

/*************************************************************
          COMB FILTERS
*************************************************************/
//buffers for the comb filters
#define NUM_COMB_FILTERS 6

#define CF0_SIZE AUDIO_SEC_TO_BLOCKS(.05)
#define CF1_SIZE AUDIO_SEC_TO_BLOCKS(.056)
#define CF2_SIZE AUDIO_SEC_TO_BLOCKS(.061)
#define CF3_SIZE AUDIO_SEC_TO_BLOCKS(.068)
#define CF4_SIZE AUDIO_SEC_TO_BLOCKS(.072)
#define CF5_SIZE AUDIO_SEC_TO_BLOCKS(.078)

L2DATA q31 cf0Data[CF0_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf0Buf(cf0Data, CF0_SIZE, &fx);

L2DATA q31 cf1Data[CF1_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf1Buf(cf1Data, CF1_SIZE, &fx);

L2DATA q31 cf2Data[CF2_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf2Buf(cf2Data, CF2_SIZE, &fx);

L2DATA q31 cf3Data[CF3_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf3Buf(cf3Data, CF3_SIZE, &fx);

L2DATA q31 cf4Data[CF4_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf4Buf(cf4Data, CF4_SIZE, &fx);

L2DATA q31 cf5Data[CF5_SIZE * (AUDIO_BUFSIZE << 1)];
static AudioRingBuf cf5Buf(cf5Data, CF5_SIZE, &fx);

static AudioRingBuf *combFilters[NUM_COMB_FILTERS] = { &cf0Buf, &cf1Buf, &cf2Buf, &cf3Buf, &cf4Buf, &cf5Buf };

/*************************************************************
          FIR FILTER
*************************************************************/

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

q31 lastBlockL[AUDIO_BUFSIZE], lastBlockR[AUDIO_BUFSIZE];

/*************************************************************
*************************************************************/

q31 dryData[AUDIO_BUFSIZE << 1];
q31 scratchDataL[AUDIO_BUFSIZE], scratchDataR[AUDIO_BUFSIZE];

//this will hold interleaved data
q31 *processData;

void processER()
{
  //process early reflections network
  q31 *p = processData;
  q31 *l = filterInputLeft;
  q31 *r = filterInputRight;

  q31 *tpl[NUM_ER_TAPS];
  q31 *tpr[NUM_ER_TAPS];

  for(int i=0; i<NUM_ER_TAPS; i++){
    tpl[i] = ERLeft + (TAP_MAX - taps[i].dl);
    tpr[i] = ERRight + (TAP_MAX - taps[i].dl);
  }

  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *l = 0;
    *r = 0;

    //process the taps
    for(int j=0; j<NUM_ER_TAPS; j++){
      *l += __builtin_bfin_mult_fr1x32x32(*tpl[j]++, taps[j].gain);
      *r += __builtin_bfin_mult_fr1x32x32(*tpr[j]++, taps[j].gain);
    }

    //add to the output
    *p++ = FRACMUL(*p, .5) + *l;
    *p++ = FRACMUL(*p, .5) + *r;

    //increment pointers
    l++;
    r++;
  }

  /* Process the comb filters. Rather than worrying about DMA
   * for these for now we will just use the core to fetch and write back the
   * data.
   */
  for(int i=0; i<NUM_COMB_FILTERS; i++){
    if(combFilters[i]->full()){
      combFilters[i]->popCore(scratchDataL, scratchDataR);

      q31 *il = filterInputLeft;
      q31 *ir = filterInputRight;
      q31 *l = scratchDataL;
      q31 *r = scratchDataR;
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

  //push to the late reflections delay buffer
  filterDelayBuf.push(scratchDataL, scratchDataR);

  //save the input samples
  AUDIO_COPY(lastBlockL, filterInputLeft);
  AUDIO_COPY(lastBlockR, filterInputRight);
}

void addLateReflections()
{
  /* Add the delayed late reflections network to
   * the output signal.
   */
  q31 *p = processData;
  q31 *l = filterLeft;
  q31 *r = filterRight;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *p++ += *l++;
    *p++ += *r++;
  }
}

//schedule the early reflections task when data is ready
void ERFetchDone() { sch.addTask(processER, SCHEDULER_MAX_PRIO); }

//schedule the filter task when the filter data is ready
void LRFetchDone() { sch.addTask(addLateReflections, SCHEDULER_MAX_PRIO - 1); }

//This will run in interrupt context
void audioHook(q31 *data)
{
  //save the pointer to the new data
  processData = data;

  //push to the early reflections ring buffer
  memcpy(dryData, data, (AUDIO_BUFSIZE << 1) * sizeof(q31));
  ERBuf.pushInterleaved(dryData);

  //begin fetching necessary data
  if(ERBuf.full()){
    ERBuf.peekHeadSamples(ERLeft, ERRight, TAP_MIN, ER_SIZE, ERFetchDone);
    ERBuf.discard();
  }

  if(filterDelayBuf.full())
    filterDelayBuf.pop(filterLeft, filterRight, LRFetchDone);
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
