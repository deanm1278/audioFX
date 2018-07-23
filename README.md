# audioFX
library for blackfin audio FX board

An audio demo of a pedal made using this code can be found [here](https://youtu.be/_QWdeb03lg8)
The code running in the above video is [this example](https://github.com/deanm1278/audioFX/blob/master/examples/pedals/multitap/multitap.ino)

This is code for creating audio effects on BF70x Blackfin+ DSP chips. A Blackfin+ processor and an I2S DAC are required. This library currently supports both the [AK4558](https://www.akm.com/akm/en/file/datasheet/AK4558EN.pdf) (recommended) and the [ADAU17x1](http://www.analog.com/media/en/technical-documentation/data-sheets/ADAU1761.pdf).

Audio buffers are handled via DMA and a callback function is specified when a buffer of data is ready to be processed.
A simple gain example is shown here:

```
#include "audioFX.h"
#include "ak4558.h"

ak4558 iface;

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
	iface.begin();

	fx.begin();

	//set the function to be called when a buffer is ready
	fx.setHook(audioLoop);
}

// the loop function runs over and over again forever
void loop() {
	__asm__ volatile ("IDLE;");
}
```

Note that some examples in this library are currently obsolete and need to be updated to reflect changes to the library.

## Dependencies
This code depends on:
* [GNU toolchain for blackfin+](https://github.com/deanm1278/blackfin-plus-gnu)
* [BF706 header files](https://github.com/deanm1278/bfin-CMSIS)
* [Blackfin+ Arduino Core](https://github.com/deanm1278/ArduinoCore-blackfin)
