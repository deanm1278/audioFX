/* for easy patching through MAX MIDI interface */

#include "MIDI.h"
#include "adau17x1.h"
#include "audioFX.h"
#include "fm.h"
#include "midi_notes.h"

using namespace FX;

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1,  MIDI);

static RAMB q31 outputDataL[AUDIO_BUFSIZE], outputDataR[AUDIO_BUFSIZE];
static uint32_t roundRobin = 0;

LFO<q31> lfoVol( _F16(2.0) );
LFO<q31> lfoVol2( _F16(2.5) );
#define LFO_VOL_DEPTH .45

#define CC_SCALE 16892410

adau17x1 iface;

void handleNoteOn(byte channel, byte pitch, byte velocity);
void handleNoteOff(byte channel, byte pitch, byte velocity);
void handleCC(byte channel, byte number, byte value);
void audioHook(q31 *data);

//********** SYNTH SETUP *****************//

#define NUM_OPERATORS 6
Operator op1(0), op2(1), op3(2), op4(3), op5(4), op6(5);
Operator *ops[NUM_OPERATORS] = {&op1, &op2, &op3, &op4, &op5, &op6};
Algorithm A(ops, NUM_OPERATORS);

#define NUM_VOICES 8
Voice v1(&A), v2(&A), v3(&A), v4(&A), v5(&A), v6(&A), v7(&A), v8(&A);
Voice *voices[] = {&v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8};

RatioFrequency op2Ratio(&op1, _F16(14.0));
RatioFrequency op4Ratio(&op1, _F16(.5));
FixedFrequency op5Freq(_F16(158.5));
RatioFrequency op6Ratio(&op1, _F16(1.003));

void chooseAlgo(uint8_t num);

void setup()
{
  chooseAlgo(3);

  //******** DEFINE MODULATOR FREQUENCIES ********//
  op2.setCarrier(&op2Ratio);
  op4.setCarrier(&op4Ratio);
  op5.setCarrier(&op5Freq);
  op6.setCarrier(&op6Ratio);

  Serial.begin(115200);

#if 0
  q31 fakedata[AUDIO_BUFSIZE*2];
  PROFILE("fm 8v6op", {
      audioHook(fakedata);
  });
#endif

  iface.begin();

  //begin fx processor
  fx.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);

  //set callbacks
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);
  MIDI.setHandleControlChange(handleCC);

  // Initiate MIDI communications, listen to all channels
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop()
{
    // Call MIDI.read the fastest you can for real-time performance.
    MIDI.read();
    __asm__ volatile("IDLE;");
}

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
  for(int i=roundRobin; i<roundRobin+NUM_VOICES; i++){
  int v = i%NUM_VOICES;
    if(!voices[v]->active){
      voices[v]->setOutput(midi_notes[pitch]);
      voices[v]->trigger(true);
      voices[v]->gain = (q31)velocity * (CC_SCALE/2);
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
        voices[v]->gain = (q31)velocity * (CC_SCALE/2);
        roundRobin++;
        return;
      }
    }
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
  for(int i=0; i<NUM_VOICES; i++){
    //look for the note that was played
    if(voices[i]->active && !voices[i]->queueStop && voices[i]->output == midi_notes[pitch]){
      voices[i]->trigger(false, true);
      break;
    }
  }
}

void handleCC(byte channel, byte number, byte value)
{
  Serial.print(number); Serial.print(", "); Serial.println(value);
  if(number >= 10){
    uint8_t op = floor( (number - 10) /15);
    uint8_t offset = (number - 10) % 15;

    switch(offset){
    case 0:
      //coarse
      break;
    case 1:
      //fine
      break;
    case 2:
      ops[op]->feedbackLevel = (q31)value * CC_SCALE;
      break;
    case 3:
      //fixed vs. ratio
      break;
    case 4:
      ops[op]->volume.attack.time = (uint16_t)value * (uint16_t)value;
      break;
    case 5:
      ops[op]->volume.attack.level = (q31)value * CC_SCALE;
      break;
    case 6:
      ops[op]->volume.decay.time =  (uint16_t)value * (uint16_t)value;
      break;
    case 7:
      ops[op]->volume.decay.level = (q31)value * CC_SCALE;
      break;
    case 8:
      ops[op]->volume.decay2.time = (uint16_t)value * (uint16_t)value;
      break;
    case 9:
      ops[op]->volume.sustain.level = (q31)value * CC_SCALE;
      break;
    case 10:
      ops[op]->volume.release.time =  (uint16_t)value * (uint16_t)value;
      break;
    case 11:
      ops[op]->volume.release.level = (q31)value * CC_SCALE;
      break;
    default:
      break;
    }
  }
  else{
    switch(number){
      case 2:
        chooseAlgo(value);
        break;

      default:
        break;
    }
  }
}


void audioHook(q31 *data)
{
  zero(outputDataL);

  for(int i=0; i<NUM_VOICES; i++)
    voices[i]->play(outputDataL);

  for(int i=0; i<AUDIO_BUFSIZE; i++) outputDataL[i] >>= 7;
  limit24(outputDataL);

  copy(outputDataR, outputDataL);

  lfoVol.getOutput(outputDataL);
  lfoVol2.getOutput(outputDataR);

  interleave(data, outputDataL, outputDataR);
}

