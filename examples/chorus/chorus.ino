/**************************************
 * This example shows how to do a simple chorus effect.
 */

#include "audioFX.h"
#include "audioRingBuf.h"
#include "scheduler.h"

//create the fx object
AudioFX fx;
Scheduler sch;

/* the maximum number of samples the chorus voice will be delayed */
#define MAX_DELAY 1024

/* how many samples will be in our LFO */
#define LFO_SAMPLES 1024

/* how many audioLoops go by before the LFO position is incremented.
 *  Each audioLoop is about 2 mS.
 */
#define RATE_LEFT 4
#define RATE_RIGHT 7

/* The multiplier for the chorus sample */
#define DEPTH .5

/* the multiplier for the dry sample */
#define DRY_MIX .6

q31 delayData[((MAX_DELAY/AUDIO_BUFSIZE) + 2) * (AUDIO_BUFSIZE*2)];
static AudioRingBuf<q31> buf(delayData, ((MAX_DELAY/AUDIO_BUFSIZE) + 2), &fx);

q31 tap0Left[AUDIO_BUFSIZE],
    tap0Right[AUDIO_BUFSIZE],

/* unused samples will be stored here */
	unused[AUDIO_BUFSIZE];

q31 *inputData;

/* this will be our LFO lookup table */
static uint16_t sine[LFO_SAMPLES];
int loopCounter;
int leftIndex, rightIndex;

/* the offset of the block we will need to grab */
int blockIndex;
int subIndex;

void processChorus()
{
  for(int i=0; i<AUDIO_BUFSIZE; i++){
	  *inputData++ = FRACMUL(*inputData, DRY_MIX) + FRACMUL(tap0Left[i], DEPTH);
	  *inputData++ = FRACMUL(*inputData, DRY_MIX) + FRACMUL(tap0Right[i], DEPTH);
  }
}

void audioHook(q31 *data)
{
	inputData = data;
	buf.push(data);
	loopCounter++;

	/* once the delay buffer has filled up we can start grabbing samples */
	if(buf.full()){

		/* increment the loop counter when the desired number of audioLoops has passed,
		but make sure it doesn't overshoot the LFO array .*/
		if(loopCounter%RATE_LEFT == 0){
		  leftIndex++;
		  leftIndex = leftIndex%LFO_SAMPLES;
		}
		if(loopCounter%RATE_RIGHT == 0){
		  rightIndex++;
		  rightIndex = rightIndex%LFO_SAMPLES;
		}

		/* get a block of samples for the left and right channels */
		buf.peekBack(unused, tap0Left, sine[leftIndex], AUDIO_BUFSIZE, NULL);
		buf.peekBack(unused, tap0Right, sine[leftIndex], AUDIO_BUFSIZE, processChorus);

		/* remove the last block in the buffer to make room for new blocks
		 *  once we have the samples we need.
		 */
		buf.discard();
	}
}

void setup() {
  loopCounter = 0;

  //start left and right chorus at different points
  leftIndex = 0;
  rightIndex = LFO_SAMPLES / 4;

  /* create a sine wave for the LFO. Each point represents the number
   *  of samples to delay the input.
   */
  for(int i=0; i<LFO_SAMPLES; i++){
    sine[i] = (MAX_DELAY/2) + (sin(2*PI/LFO_SAMPLES*i)*(MAX_DELAY/2-1));
  }

  fx.begin();
  sch.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);

  sch.addTask(loop, 0);
}

void loop() {
	__asm__ volatile("IDLE;");
}
