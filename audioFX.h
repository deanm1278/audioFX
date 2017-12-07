/*
 * audioFX.h
 *
 *  Created on: Dec 6, 2017
 *      Author: Dean
 */

#ifndef LIB_AUDIOFX_H_
#define LIB_AUDIOFX_H_

#include "I2S.h"

#define AUDIO_BUFSIZE 128

class AudioFX : public I2S
{
public:
	AudioFX( void );
	bool begin( void );
	void processBuffer( void );
	void setCallback( void (*fn)(int32_t *) );

private:
	void (*audioCallback)(int32_t *);
};


#endif /* LIB_AUDIOFX_H_ */
