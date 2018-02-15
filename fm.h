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

    virtual T getOutput() { return this->output; };
    void setOutput(T out) { output = out; }

    bool calculated;
protected:
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

    T getOutput(Voice *voice);

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
    q31 getOutput(Voice *voice);

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

    q16 getOutput(){
        output = _mult_q16(source->carrier->getOutput(), ratio);
        return output;
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
    q31 getOutput(Voice *voice);

private:
    Operator **ops;
    uint8_t numOps;
};

/************* VOICE CLASS **************/
enum {
    FM_ENVELOPE_TRIGGER_NONE = 0,
    FM_ENVELOPE_TRIGGER_TICK,
};

class Voice : public Modulator<q16>{
public:
    Voice(Algorithm *algo) { algorithm = algo; t = 0; active = false; triggerEnvelopes = FM_ENVELOPE_TRIGGER_NONE; }
    ~Voice() {}

    q28 getT() { return t; }
    q31 play() {
        if(t % (FM_INC * 50) == 0){
            ms++;
            triggerEnvelopes = FM_ENVELOPE_TRIGGER_TICK;
        }

        q31 result = algorithm->getOutput(this);
        //increment the timestep for this voice
        t = (t + FM_INC) & ~(_F28_INTEGER_MASK << 1);

        triggerEnvelopes = FM_ENVELOPE_TRIGGER_NONE;
        return result;
    }
    void trigger(bool state) {
        if(state)
            t = 0;
        active = state;
        ms = 0xFFFFFFFF;
    }

    volatile bool active;
    int triggerEnvelopes;
    volatile uint32_t ms; //millisecond counter for envelopes
private:
    q28 t;
    Algorithm *algorithm;
};


#endif /* AUDIOFX_FM_H_ */
