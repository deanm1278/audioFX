/*
 * A Simple FM sketch
 */

#include "audioFX.h"

//create the fx object
AudioFX fx;

q16 freq1 = _F16(440);
q16 freq2 = _F16(880.5);

q16 imod = _F16(2.0);

void audioHook(q31 *data)
{
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    *data++ = fm(freq1, freq2, imod);
    *data++ = fm(freq1, freq2, imod);

    fm_tick();
  }
}

void setup() {
  fx.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);
}

void loop() {
  __asm__ volatile("IDLE;");
}

