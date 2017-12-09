/**************************************
 * This example shows how to do a multi-tap delay.
 * A big circular buffer in L2 SRAM is used.
 * 
 * Non-blocking Buffer fetches via MDMA (memory-to-memory DMA)
 * as well as blocking buffer fetches through the core are used.
 */

#include "audioFX.h"
#include "audioRingBuf.h"

//create the fx object
AudioFX fx;

/*
 * Create a ring buffer in L2 RAM to hold the delay samples
 * (we have lots of L2 SRAM). The length of the delay will be
 * dictated by the size of the buffer.
 */
AudioRingBuf buf(128, &fx);

bool newBufferReceived;

/*
 * These buffers will hold the samples we fetch from L2 SRAM.
 */
int32_t tap0Left[AUDIO_BUFSIZE],
    tap0Right[AUDIO_BUFSIZE];

int32_t tap1Left[AUDIO_BUFSIZE],
    tap1Right[AUDIO_BUFSIZE];

void audioLoop(int32_t *left, int32_t *right)
{
  /* Add the fetched blocks from the big L2 buffer. 
   * They will fade a little each time.
   */
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    left[i] = left[i] +  tap1Left[i] * .3 + tap0Left[i] * .3;
    right[i] = right[i] + tap1Right[i] * .3 + tap0Right[i] * .3;
  }

  /* push the processed buffer to the back of the
   *  L2 circular buffer.
   */
  buf.push(left, right);
  newBufferReceived = true;
}

void setup() {
  fx.begin();
  fx.setCallback(audioLoop);
  newBufferReceived = false;
}

void loop() {
  //process our buffer if one is available
  fx.processBuffer();
  
  if(newBufferReceived){
    /* A new buffer has just been processed!
     *  Lets fetch the buffers the audioLoop will use for the next
     *  block now so we have plenty of time to process when we get new data.
     *  Once the delay buffer has filled up, we will start popping
     *  blocks off the end.
     */
    if(buf.full()){
      /* this is a non-blocking fetch. Grab the last tap via DMA */
      buf.pop(tap1Left, tap1Right);

      /* since our MDMA channels are being used by the previous fetch,
       *  lets fetch the other tap through the core since it's idle.
       *  This is taken from the middle of the buffer (tail - 40 blocks)
       */
      buf.peekCore(tap0Left, tap0Right, 40);
    }
    newBufferReceived = false;
  }

}
