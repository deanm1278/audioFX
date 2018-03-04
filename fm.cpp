/*
 * fm.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: deanm
 */

#include "fm.h"

Operator::Operator() : volume() {
    isOutput = false;
    carrierOverride = false;
    carrier = NULL;
    feedbackLevel = 0;

    for(int i=0; i<OP_MAX_INPUTS; i++)
        mods[i] = NULL;
}

void Operator::setCarrier(Modulator<q16> *mod)
{
	if(mod == NULL) carrierOverride = false;
	else{
		carrierOverride = true;
		carrier = mod;
	}
}

// calculate the output and add it to buf
void Operator::getOutput(q31 *buf, Voice *voice) {

    if(!calculated){

    	//calculate modulators
    	q31 mod_buf[AUDIO_BUFSIZE];
    	memset(mod_buf, 0, AUDIO_BUFSIZE*sizeof(q31));

        for(int ix=0; ix<OP_MAX_INPUTS; ix++){
            if(mods[ix] != NULL)
            	mods[ix]->getOutput(mod_buf, voice);
        }

        //calculate envelope
        q31 volume_buf[AUDIO_BUFSIZE];
        volume.getOutput(volume_buf, voice);

        q28 t = voice->getT();
        q16 cfreq[AUDIO_BUFSIZE];
        carrier->getOutput(cfreq);

        if(feedbackLevel == 0)
        	_fm_modulate(t, buf, cfreq, mod_buf, volume_buf);
        else
        	_fm_modulate_feedback(t, buf, cfreq, volume_buf, feedbackLevel, &voice->lastFeedback);

        calculated = true;
    }
}

void Algorithm::getOutput(q31 *buf, Voice *voice) {
    for(int i=0; i<numOps; i++){
        ops[i]->calculated = false;
        if(ops[i]->isOutput && !ops[i]->carrierOverride) ops[i]->carrier = voice;
    }

    for(int i=0; i<numOps; i++){
        //set carriers to the voice frequency and calculate
        if(ops[i]->isOutput){
            ops[i]->getOutput(buf, voice);
        }
    }
}

template <>
void Envelope<q31>::setDefaults(){
    attack  = { _F(.999), 0 };
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
	T start[3] = {0, 0, 0};
	T inc[3] = {0, 0, 0};
	for(int i=-1; i<2; i++){
		int idx = i + 1;
		int32_t ms = voice->ms + i;
        //tick the envelope
		if(!voice->active){
			if(voice->ms > release.time){
				start[idx] = release.level;
				inc[idx] = 0;
			}
			else{
				//we are in R
				if(ms == -1)
					start[idx] = sustain.level;
				else
					start[idx] = sustain.level + (release.level - sustain.level)/release.time * ms;
				inc[idx] = (release.level - sustain.level)/release.time;
			}
		}
		else{
            //we are in ADS
            if(ms >= attack.time + decay.time){
                //Sustain
                start[idx] = sustain.level;
                inc[idx] = 0;
            }
            else if(ms > attack.time){
                //Decay
            	start[idx] = attack.level + (decay.level - attack.level)/decay.time * (ms - attack.time);
            	inc[idx] = (decay.level - attack.level)/decay.time;
            }
            else{
                //Attack
            	if(ms == -1)
            		start[idx] = 0;
            	else
					start[idx] = attack.level/attack.time * ms;
				inc[idx] = attack.level/attack.time;
            }
        }
		inc[idx] = inc[idx]/(AUDIO_BUFSIZE/2);
    }

	for(int i=1; i<=2; i++){
		*ptr++ = start[i-1];
		for(int j=1; j<AUDIO_BUFSIZE/2; j++)
			*ptr++ = *(ptr-1) + inc[i];
	}
}
