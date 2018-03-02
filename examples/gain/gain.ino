#include "audioFX.h"

/*this function will be called when there is a new buffer of data ready
 * to be processed. Make sure not to block in here!
 */
void audioLoop(int32_t *data)
{
	for(int i=0; i<AUDIO_BUFSIZE; i++){
		*data++ = *data / 2;
		*data++ = *data / 2;
	}
}

// the setup function runs once when you press reset or power the board
void setup() {
	fx.begin();

	//set the function to be called when a buffer is ready
	fx.setHook(audioLoop);
}

// the loop function runs over and over again forever
void loop() {
	__asm__ volatile ("IDLE;");
}