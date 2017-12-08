#include "audioFX.h"

//create out object
AudioFX fx;

/*this function will be called when there is a new buffer of data ready
 * to be processed. Make sure not to block in here!
 */
void audioLoop(int32_t *left, int32_t *right)
{
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		//divide the left and right channels by 2
		left[i] = left[i] / 2;
		right[i] = right[i] / 2;
	}
}

// the setup function runs once when you press reset or power the board
void setup() {
	Serial.begin(115200);
	fx.begin();
	
	//set the function to be called when a buffer is ready
	fx.setCallback(audioLoop);
	
	Serial.println("Audio FX basic demo!");
}

// the loop function runs over and over again forever
void loop() {
	//this calls our callback if there is a buffer ready. Call as fast as possible!
	fx.processBuffer();
}

