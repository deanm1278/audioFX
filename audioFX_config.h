/*
 * audioFX_config.h
 *
 *  Created on: Dec 14, 2017
 *      Author: Dean
 */

#ifndef AUDIOFX_AUDIOFX_CONFIG_H_
#define AUDIOFX_AUDIOFX_CONFIG_H_

#define AUDIO_BUFSIZE 128
#define AUDIO_ENVELOPE_SIZE 64

#define AUDIO_SAMPLE_RATE 48800

//DON'T CHANGE THESE!!
#define FM_LOOKUP_INC 44005 //_F(1/48800)
#define FM_LOOKUP_MOD 0x7FFFF

#endif /* AUDIOFX_AUDIOFX_CONFIG_H_ */
