#include "fft.h"

#include "testWaveform.h"

#define FFT_SIZE   256

complex_q31  out[FFT_SIZE];

q31       op[FFT_SIZE];

FFT fft;

void setup() {
  Serial.begin(9600);

  //transform to frequency domain
  fft.fft(sine, out, FFT_SIZE);

  //and back to time domain
  fft.ifft(out, op, FFT_SIZE);

  for(int i=0; i<FFT_SIZE; i++){
    Serial.print(sine[i]);
    Serial.print(",");
    Serial.println(op[i]);
  }
}

void loop() {
  while(1) __asm__ volatile("IDLE;");
}