#include "audioFX.h"

AudioFX fx;

void audioLoop(int32_t *buf)
{
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    buf[i] = buf[i] / 2;
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  fx.begin();
  fx.setCallback(audioLoop);
}

// the loop function runs over and over again forever
void loop() {
  fx.processBuffer();
  if(Serial.available()){
    char c = Serial.read();
    Serial.print(c);
  }
}

