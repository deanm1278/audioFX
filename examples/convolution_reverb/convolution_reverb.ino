/**************************************
 * This example shows how to do a simple chorus effect.
 */

#include "audioFX.h"
#include "audioRingBuf.h"
#include "impulse_response.h"

//create the fx object
AudioFX fx;

//the number of blocks in the impulse response
#define TOTAL_BLOCKS (uint32_t)(sizeof(impulse_response)/(AUDIO_BUFSIZE<<3))

//this will hold past input data
AudioRingBuf buf(TOTAL_BLOCKS, &fx);

q31 blockDataL[AUDIO_BUFSIZE], blockDataR[AUDIO_BUFSIZE];


q31 *lptr, *rptr, *irptr;

void audioLoop(q31 *left, q31 *right)
{
  //save the input data
  buf.push(left, right);

  if(buf.full()){
    irptr = impulse_response;
    for(uint32_t i=0; i<TOTAL_BLOCKS; i++){

      //grab the next block of recorded data
      buf.peekCore(blockDataL, blockDataR, i);

      lptr = left; rptr = right;
      for(int j=0; j<AUDIO_BUFSIZE; j++){
        *lptr += __builtin_bfin_mult_fr1x32x32(*lptr, *irptr++);
        *rptr += __builtin_bfin_mult_fr1x32x32(*rptr, *irptr++);
        lptr++; rptr++;
      }
    }
    //shift out the last block in the buffer to make room for new
    buf.discard();
  }
}

void setup() {
  fx.begin();
  fx.setCallback(audioLoop);
}

void loop() {
  fx.processBuffer();
}

