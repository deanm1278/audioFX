/*
 * fm.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: deanm
 */

#include "fm.h"

Operator::Operator() : volume() {
    isCarrier = false;

    for(int i=0; i<OP_MAX_INPUTS; i++)
        mods[i] = NULL;
}

// calculate the output and add it to buf
void Operator::getOutput(q31 *buf, Voice *voice) {

    if(!calculated){
        if(isCarrier) carrier = voice;

    	//calculate modulators
    	q31 mod_buf[AUDIO_BUFSIZE];
    	memset(mod_buf, 0, AUDIO_BUFSIZE*sizeof(q31));

        for(int ix=0; ix<OP_MAX_INPUTS; ix++){
            if(mods[ix] != NULL)
            	mods[ix]->getOutput(mod_buf, voice);
        }

        //scale down by pi and to q28
    	q31 *ptr = mod_buf;
    	for(int i=0; i<AUDIO_BUFSIZE; i++)
        	*ptr++ = __builtin_bfin_mult_fr1x32x32(*ptr, _F( (1.0/PI)*.25) );

        //calculate envelope
        q31 volume_buf[AUDIO_BUFSIZE];
        volume.getOutput(volume_buf, voice);

        q28 t = voice->getT();
        q28 x;

        q16 cfreq[AUDIO_BUFSIZE];
        carrier->getOutput(cfreq);

        ptr = buf;

        for(int i=0; i<AUDIO_BUFSIZE; i++){

			x = _mult_q28xq16_mod(t, cfreq[i]);

			x *= 4; //multiply by 2 and scale up to units of pi/2 (multiply by 2 again)

			q31 result = sin_q28(x + mod_buf[i]);

	        //add to the output
	        *ptr++ = __builtin_bfin_add_fr1x32(*ptr, __builtin_bfin_mult_fr1x32x32(result, volume_buf[i]));

	        //increment the time step
	        t = (t + FM_INC) & ~(_F28_INTEGER_MASK << 1);
        }
        calculated = true;
    }
}

void Algorithm::getOutput(q31 *buf, Voice *voice) {
    for(int i=0; i<numOps; i++)
        ops[i]->calculated = false;

    for(int i=0; i<numOps; i++){
        //set carriers to the voice frequency and calculate
        if(ops[i]->isCarrier){
            ops[i]->getOutput(buf, voice);
        }
    }

    //gain down
    q31 gain = voice->gain;
    q31 *ptr = buf;

    for(int i=0; i<AUDIO_BUFSIZE; i++)
    	*ptr++ = __builtin_bfin_mult_fr1x32x32(*ptr, gain);
}

template <>
void Envelope<q31>::setDefaults(){
    attack  = { _F(0), 0 };
    decay   = { _F(0), 0 };
    sustain = { _F(.999), 0 };
    release = { _F(0), 0 };
}

template <>
void Envelope<q16>::setDefaults(){
    attack  = { _F16(0), 0 };
    decay   = { _F16(0), 0 };
    sustain = { _F16(0), 0 };
    release = { _F16(0), 0 };
}

template <class T>
void Envelope<T>::getOutput(T *buf, Voice *voice){

	//for now calculate the envelope twice per buffer
	T *ptr = buf;
	for(int i=0; i<2; i++){
		T result;
		uint32_t ms = voice->ms + i;
        //tick the envelope
		if(!voice->active){
			if(voice->ms > release.time){
				result = release.level;
			}
			else{
				//we are in R
				result = sustain.level + (release.level - sustain.level)/release.time * ms;
			}
		}
		else{
            //we are in ADS
            if(ms >= attack.time + decay.time){
                //Sustain
                result = sustain.level;
            }
            else if(ms >= attack.time){
                //Decay
            	result = attack.level + (decay.level - attack.level)/decay.time * (ms - attack.time);
            }
            else{
                //Attack
            	result = attack.level/attack.time * ms;
            }
        }

        for(int j=0; j<AUDIO_BUFSIZE/2; j++)
        	*ptr++ = result;
    }
}
