/**************************************
 * This example shows how to do a simple chorus effect.
 */

#include "audioFX.h"
#include "audioRingBuf.h"

//create the fx object
AudioFX fx;

/* the maximum number of samples the chorus voice will be delayed */
#define MAX_DELAY 1024

/* how many samples will be in our LFO */
#define LFO_SAMPLES 256

/* how many audioLoops go by before the LFO position is incremented.
 *  Each audioLoop is about 2 mS.
 */
#define LFO_DIV 12

/* The multiplier for the chorus sample */
#define DEPTH .1

/* the multiplier for the dry sample */
#define DRY_MIX .6

AudioRingBuf buf((MAX_DELAY/AUDIO_BUFSIZE) + 1, &fx);
bool newBufferReceived;

int32_t tap0Left[AUDIO_BUFSIZE*2],
    tap0Right[AUDIO_BUFSIZE*2];

int32_t dryLeft[AUDIO_BUFSIZE],
	dryRight[AUDIO_BUFSIZE];

/* this will be our LFO lookup table */
uint16_t sine[LFO_SAMPLES];
int loopCounter;
int lfoIndex;

/* the offset of the block we will need to grab */
int blockIndex;
int subIndex;

void audioLoop(q31 *left, q31 *right)
{
  //copy the dry samples to the end of our delay buffer since they're the most recent
  memcpy(dryLeft, left, AUDIO_BUFSIZE * sizeof(int32_t));
  memcpy(dryRight, right, AUDIO_BUFSIZE * sizeof(int32_t));

  buf.push(dryLeft, dryRight);

  if(blockIndex == 0){
	  memcpy(tap0Left + AUDIO_BUFSIZE, left, AUDIO_BUFSIZE * sizeof(int32_t));
	  memcpy(tap0Right + AUDIO_BUFSIZE, right, AUDIO_BUFSIZE * sizeof(int32_t));
  }

  subIndex = AUDIO_BUFSIZE - sine[lfoIndex]%AUDIO_BUFSIZE;
  for(int i=0; i<AUDIO_BUFSIZE; i++){
      left[i] = FRACMUL(left[i], DRY_MIX) + FRACMUL(tap0Left[subIndex + i], DEPTH);
      right[i] = FRACMUL(right[i], DRY_MIX) + FRACMUL(tap0Right[subIndex + i], DEPTH);
  }

  loopCounter++;
  newBufferReceived = true;
}

void setup() {
  loopCounter = 0;
  lfoIndex = 0;

  /* create a sine wave for the LFO. Each point represents the number
   *  of samples to delay the input.
   */
  for(int i=0; i<LFO_SAMPLES; i++){
    sine[i] = (MAX_DELAY/2) + (sin(2*PI/LFO_SAMPLES*i)*(MAX_DELAY/2-1));
  }

  fx.begin();
  fx.setCallback(audioLoop);
  newBufferReceived = false;
}

void loop() {
  fx.processBuffer();

  if(newBufferReceived){
    /* once the delay buffer has filled up we can start grabbing samples */
    if(buf.full()){

      /* increment the loop counter when the desired number of audioLoops has passed,
      but make sure it doesn't overshoot the LFO array .*/
      if(loopCounter%LFO_DIV == 0){
        lfoIndex++;
        lfoIndex = lfoIndex%LFO_SAMPLES;
      }

      /* find which block the first sample is in. */
      blockIndex = floor(sine[lfoIndex]/AUDIO_BUFSIZE);

      /* we will need 2 blocks of samples.
       *  for example, if sine[lfoIndex] = 220,
       *  we will need samples 220 through 220 + 128.
       *  that would be blocks 1 and 2 since each block contains
       *  128 samples.
       */
      buf.peekHeadCore(tap0Left, tap0Right, blockIndex);
      if(blockIndex > 0)
    	  buf.peekHeadCore(tap0Left + AUDIO_BUFSIZE, tap0Right + AUDIO_BUFSIZE, blockIndex - 1);

      /* remove the last block in the buffer to make room for new blocks
       *  once we have the samples we need.
       */
      buf.discard();
    }
    newBufferReceived = false;
  }

}
