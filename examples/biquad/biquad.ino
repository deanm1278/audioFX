#include "audioFX.h"

/***** filter coefficients *******/
#define A0 .7
#define A1 .001
#define A2 -.6

#define B1 .1
#define B2 .8
/********************************/

//create the fx object
AudioFX fx;

bool newBufferReceived;

q31 lastin = 0;
q31 lastin2 = 0;
q31 lastout = 0;
q31 lastout2 = 0;

void audioLoop(q31 *left, q31 *right)
{
  /* https://en.wikipedia.org/wiki/Digital_biquad_filter
   *
   * y[n] = a0*x[n] + a1*x[n-1] + a2*x[n-2] - b1*y[n-1] - b2*y[n-2]
   */
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    lastin2 = lastin;
    lastin = left[i];
    lastout2 = lastout;
    left[i] = FRACMUL(left[i], A0) + FRACMUL(lastin, A1) + FRACMUL(lastin2, A2) - FRACMUL(lastout, B1) - FRACMUL(lastout2, B2);

      right[i] = left[i]; //mono

      lastout = left[i];
  }
}

void setup() {
  fx.begin();
  fx.setCallback(audioLoop);
}

void loop() {
  fx.processBuffer();
}

