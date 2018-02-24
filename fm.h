/*
 * fm.h
 *
 *  Created on: Feb 12, 2018
 *      Author: deanm
 */

#ifndef AUDIOFX_FM_H_
#define AUDIOFX_FM_H_

#include "utility.h"

#define FM_NUM_VOICES 8
#define OP_MAX_INPUTS 4
#define FM_MAX_OPERATORS 6
#define FM_MAX_ENVELOPES FM_MAX_OPERATORS

class Voice;
class Operator;

/************* BASE MODULATOR CLASS **************/
template<class T> class Modulator {
public:
    Modulator(T initialOutput = 0){ this->output = initialOutput; this->calculated = false; }
    ~Modulator() {}

    virtual void getOutput(T *buf) { for(int i=0; i<AUDIO_BUFSIZE; i++) *buf++ = output; };
    void setOutput(T out) { output = out; }

    bool calculated;
    T output;
};
template class Modulator<q31>;
template class Modulator<q16>;

/************* FIXED FREQUENCY CLASS **************/
class FixedFrequency : public Modulator<q16> {
public:
    FixedFrequency(q16 freq) { output = freq; }
    ~FixedFrequency() {}
};

/************* ENVELOPE CLASS **************/
template<class T> class Envelope : public Modulator<T> {
public:
    Envelope() { setDefaults(); }
    ~Envelope() {}

    void getOutput(T *buf, Voice *voice);

    void setDefaults();

    typedef struct {
        T level;
        uint32_t time;
    } envParam;

    envParam attack,
             decay,
             sustain,
             release;
private:

};
template class Envelope<q31>;
template class Envelope<q16>;

/************* OPERATOR CLASS **************/
class Operator : public Modulator<q31> {
public:
    Operator();
    ~Operator() {}

    /* runs FM equation. Calculates operators this on depends on if they are not done
     * requires the current status of any envelopes.
     * If a circular reference is encountered, the previous calculation is used
     */
    void getOutput(q31 *buf, Voice *voice);

    Operator *mods[OP_MAX_INPUTS];

    Modulator<q16> *carrier;

    Envelope<q31> volume;

    bool isCarrier;
private:
};

/************* RATIO FREQUENCY CLASS **************/
class RatioFrequency : public Modulator<q16> {
public:
    RatioFrequency(Operator *source, q16 ratio) {
        this->source = source;
        this->ratio = ratio;
    }
    ~RatioFrequency() {}

    //TODO: this can be optimized if carrier is always static
    void getOutput(q16 *buf){
    	source->carrier->getOutput(buf);

    	for(int i=0; i<AUDIO_BUFSIZE; i++)
    		*buf++ = _mult_q16(*buf, ratio);
    }

    Operator *source;
    q16 ratio;

};

/************* ALGORITHM CLASS **************/
class Algorithm : public Modulator<q31>{
public:
    Algorithm(Operator **operators, uint8_t numOperators) { ops = operators; numOps = numOperators; }
    ~Algorithm() {}

    /* runs all operators for a given voice, return mixed output */
    void getOutput(q31 *buf, Voice *voice);

private:
    Operator **ops;
    uint8_t numOps;
};

/************* VOICE CLASS **************/

class Voice : public Modulator<q16>{
public:
    Voice(Algorithm *algo) {
    	algorithm = algo;
    	t = 0;
    	active = false;
    	ms = 0;
    	gain = _F(.999);
    }
    ~Voice() {}

    q28 getT() { return t; }
    void play(q31 *buf, q31 gain) {
    	this->gain = gain;
    	algorithm->getOutput(buf, this);

    	//increment time TODO: this is not great as non-integer frequencies don't line up right
    	t = (t + FM_INC*AUDIO_BUFSIZE) & ~(_F28_INTEGER_MASK << 1);
    	ms += 2;
    }
    void trigger(bool state) {
        if(state)
            t = 0;
        active = state;
        ms = 0;
    }

    volatile bool active;
    uint32_t ms;
    q31 gain;

private:
    q28 t;
    Algorithm *algorithm;
};


#endif /* AUDIOFX_FM_H_ */
