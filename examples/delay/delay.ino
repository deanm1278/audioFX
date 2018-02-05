/**************************************
 * This example shows how to do a multi-tap delay.
 * A big circular buffer in L2 SRAM is used.
 *
 * Non-blocking Buffer fetches via MDMA (memory-to-memory DMA) are used.
 */

#include "audioFX.h"
#include "audioRingBuf.h"

//create the fx object
AudioFX fx;

#define DRY_GAIN .7
#define TAP0_FEEDBACK .4
#define TAP1_FEEDBACK .3

#define DELAY_TIME_MAX AUDIO_SEC_TO_BLOCKS(1)
#define DELAY_TIME_TAP0 AUDIO_SEC_TO_BLOCKS(.6)

/*
 * Create a ring buffer in L2 RAM to hold the delay samples
 * (we have lots of L2 SRAM). The length of the delay will be
 * dictated by the size of the buffer.
 */
L2DATA q31 delayData[DELAY_TIME_MAX*AUDIO_BUFSIZE*2];
AudioRingBuf<q31> buf(delayData, DELAY_TIME_MAX, &fx);

/*
 * These buffers will hold the samples we fetch from L2 SRAM.
 */
int32_t tap0Left[AUDIO_BUFSIZE], tap0Right[AUDIO_BUFSIZE],
		tap1Left[AUDIO_BUFSIZE], tap1Right[AUDIO_BUFSIZE];

q31 *currentData;

void dataReady(){
   /* Add the fetched blocks from the big L2 buffer.
   * They will fade a little each time.
   */
  q31 *dPtr = currentData;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
	*dPtr++ = FRACMUL(*dPtr, DRY_GAIN) +  FRACMUL(tap1Left[i], TAP1_FEEDBACK) + FRACMUL(tap0Left[i], TAP0_FEEDBACK);
	*dPtr++ = FRACMUL(*dPtr, DRY_GAIN) +  FRACMUL(tap1Right[i], TAP1_FEEDBACK) + FRACMUL(tap0Right[i], TAP0_FEEDBACK);
  }

  /* push the processed buffer to the back of the
   *  L2 circular buffer.
   */
  buf.push(currentData);
}

void audioHook(int32_t *data)
{
  currentData = data;

  if(buf.full()){
      /* this is a non-blocking fetch. Grab the last tap via DMA */
      buf.peek(tap0Left, tap0Right, DELAY_TIME_TAP0);
      buf.pop(tap1Left, tap1Right, dataReady);
  }
  else buf.push(data);
}

void setup() {
  fx.begin();
  fx.setHook(audioHook);
}

void loop() {
	__asm__ volatile("IDLE;");
}
