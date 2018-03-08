/* This is a sample organ patch with vibrato effect.
 * 8 voices, 5 operators
 */

#include "MIDI.h"
#include "audioFX.h"
#include "fm.h"
#include "midi_notes.h"


MIDI_CREATE_INSTANCE(HardwareSerial, Serial1,  MIDI);

volatile bool bufferReady;
q31 *dataPtr;

static RAMB q31 outputData[AUDIO_BUFSIZE];
static uint32_t roundRobin = 0;

LFO<q31> lfo( _F16(1.5) );
#define LFO_DEPTH 0.5

void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
void audioHook(q31 *data);

//********** SYNTH SETUP *****************//

#define NUM_OPERATORS 6
Operator op1, op2, op3, op4, op5, op6;
Operator *ops[NUM_OPERATORS] = {&op1, &op3, &op2, &op4, &op5, &op6};
Algorithm A(ops, NUM_OPERATORS);

#define NUM_VOICES 8
Voice v1(&A), v2(&A), v3(&A), v4(&A), v5(&A), v6(&A), v7(&A), v8(&A);
Voice *voices[NUM_VOICES] = {&v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8};

RatioFrequency op2Ratio(&op1, _F16(2.0));
RatioFrequency op3Ratio(&op1, _F16(1.0 + RATIO_FIFTH));
RatioFrequency op4Ratio(&op1, _F16(3.0));
RatioFrequency op5Ratio(&op1, _F16(3.0 + RATIO_FIFTH));
RatioFrequency op6Ratio(&op1, _F16(0.5));

void setup()
{
   /******* DEFINE STRUCTURE *******
   *             /\
   *           OP1 OP2 OP3 OP4/
   *              \ \  /  /
   *               output
   */
  op1.isOutput = true;
  op2.isOutput = true;
  op3.isOutput = true;
  op4.isOutput = true;
  op5.isOutput = true;
  op6.isOutput = true;

  //******** DEFINE MODULATOR FREQUENCIES ********//
  op2.setCarrier(&op2Ratio);
  op3.setCarrier(&op3Ratio);
  op4.setCarrier(&op4Ratio);
  op5.setCarrier(&op5Ratio);
  op6.setCarrier(&op6Ratio);
  //op4.feedbackLevel = _F(.7);

  //******* DEFINE ENVELOPES *******//

  //***** OP1 ENVELOPE *****//
  op1.volume.attack.time = 2;
  op1.volume.attack.level = _F(.18);

  op1.volume.decay.time = 25;
  op1.volume.decay.level = _F(.15);

  op1.volume.sustain.level = _F(.15);

  op1.volume.release.time = 25;

  //***** OP2 ENVELOPE *****//
  op2.volume.attack.time = 2;
  op2.volume.attack.level = _F(.18);

  op2.volume.decay.time = 25;
  op2.volume.decay.level = _F(.15);

  op2.volume.sustain.level = _F(.15);

  op2.volume.release.time = 25;

  //***** OP3 ENVELOPE *****//
  op3.volume.attack.time = 2;
  op3.volume.attack.level = _F(.18);

  op3.volume.decay.time = 25;
  op3.volume.decay.level = _F(.15);

  op3.volume.sustain.level = _F(.15);

  op3.volume.release.time = 25;

  //***** OP4 ENVELOPE *****//
  op4.volume.attack.time = 2;
  op4.volume.attack.level = _F(.18);

  op4.volume.decay.time = 25;
  op4.volume.decay.level = _F(.15);

  op4.volume.sustain.level = _F(.15);

  op4.volume.release.time = 25;

  //***** OP5 ENVELOPE *****//
  op5.volume.attack.time = 2;
  op5.volume.attack.level = _F(.18);

  op5.volume.decay.time = 25;
  op5.volume.decay.level = _F(.15);

  op5.volume.sustain.level = _F(.15);

  op5.volume.release.time = 25;

  //***** OP6 ENVELOPE *****//
  op6.volume.attack.time = 2;
  op6.volume.attack.level = _F(.18);

  op6.volume.decay.time = 25;
  op6.volume.decay.level = _F(.15);

  op6.volume.sustain.level = _F(.15);

  op6.volume.release.time = 25;

  lfo.depth = _F(LFO_DEPTH);
  lfo.trigger(true);

  //***** END OF PATCH SETUP *****//

  Serial.begin(9600);

  bufferReady = false;

  //begin fx processor
  fx.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);

  //set callbacks
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);

    // Initiate MIDI communications, listen to all channels
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
    // Call MIDI.read the fastest you can for real-time performance.
    MIDI.read();

    if(bufferReady){

    memset(outputData, 0, AUDIO_BUFSIZE*sizeof(q31));

    for(int i=0; i<NUM_VOICES; i++)
      voices[i]->play(outputData, _F(.99/(NUM_VOICES-4)));

    lfo.getOutput(outputData);

    q31 *ptr = outputData;
    for(int i=0; i<AUDIO_BUFSIZE; i++){
      *dataPtr++ = *ptr;
      *dataPtr++ = *ptr++;
    }

    bufferReady = false;
    }
    __asm__ volatile("IDLE;");
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  for(int i=roundRobin; i<roundRobin+NUM_VOICES; i++){
  int v = i%NUM_VOICES;
    if(!voices[v]->active){
      voices[v]->setOutput(midi_notes[pitch]);
      voices[v]->trigger(true);
      roundRobin++;
      return;
    }
  }

  //look for an interruptable voice (one where all envelopes are passed first decay stage)
  for(int i=roundRobin; i<roundRobin+NUM_VOICES; i++){
    int v = i%NUM_VOICES;
      if(voices[v]->interruptable){
        voices[v]->setOutput(midi_notes[pitch]);
        voices[v]->trigger(true);
        roundRobin++;
        return;
      }
    }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  for(int i=0; i<NUM_VOICES; i++){
    //look for the note that was played
    if(voices[i]->active && voices[i]->output == midi_notes[pitch]){
      voices[i]->trigger(false);
      break;
    }
  }
}

void audioHook(q31 *data)
{
  dataPtr = data;
  bufferReady = true;
}

