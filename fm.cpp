/*
 * fm.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: deanm
 */

#include "fm.h"

Operator::Operator() {
    isCarrier = false;

    for(int i=0; i<OP_MAX_INPUTS; i++)
        mods[i].mod = NULL;
}

q31 Operator::getOutput(Voice *voice) {

    if(!calculated){
        if(isCarrier) carrier = voice;

        q28 x = voice->getT();

        q16 cfreq = carrier->getOutput();

        if(cfreq == 0) __asm__ volatile("EMUEXCPT;");

        x = _mult_q28xq16_mod(x, cfreq);
        x = x*4; //multiply by 2 and scale up to units of pi/2 (multiply by 2 again)

        q28 mod_total = 0;
        for(int ix=0; ix<OP_MAX_INPUTS; ix++){
            if(mods[ix].mod != NULL){
                //scale down by pi
                q31 mod_out = __builtin_bfin_mult_fr1x32x32(mods[ix].mod->getOutput(voice), _F(1.0/PI));

                //multiply by imod
                mod_out = __builtin_bfin_mult_fr1x32x32(mod_out, mods[ix].env.getOutput(voice));

                //scale to q28
                mod_out = mod_out / (1<<2);

                //add to total
                mod_total += mod_out;
            }
        }

        //calculate
        q31 result = sin_q28(x + mod_total);

        output = result;
        calculated = true;
    }

    return output;
}

q31 Algorithm::getOutput(Voice *voice) {
    for(int i=0; i<numOps; i++)
        ops[i]->calculated = false;

    q31 total = 0;
    for(int i=0; i<numOps; i++){
        //set carriers to the voice frequency and calculate
        if(ops[i]->isCarrier){
            total += ops[i]->getOutput(voice);
        }
    }

    return total;
}

template <>
void Envelope<q31>::setDefaults(){
    attack  = { _F(0), _F(0) };
    decay   = { _F(0), _F(0) };
    sustain = { _F(0), _F(0) };
    release = { _F(0), _F(0) };
}

template <>
void Envelope<q16>::setDefaults(){
    attack  = { _F16(0), _F16(0) };
    decay   = { _F16(0), _F16(0) };
    sustain = { _F16(0), _F16(0) };
    release = { _F16(0), _F16(0) };
}

template <class T>
T Envelope<T>::getOutput(Voice *voice){
    if(voice->triggerEnvelopes == FM_ENVELOPE_TRIGGER_ATTACK){
        //start the envelope over
    }
    else if(voice->triggerEnvelopes == FM_ENVELOPE_TRIGGER_RELEASE){
        //go into release mode
    }
    else{

    }

    return _F(.5);
}