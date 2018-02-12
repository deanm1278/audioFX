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

/************* BASE MODULATOR CLASS **************/
template<class T> class Modulator {
public:
    Modulator();
    ~Modulator() {}

    T getOutput();
    void setOutput(T out) { output = out; }
protected:
    T output;
    bool calculated;
};
template class Modulator<q31>;
template class Modulator<q16>;

/************* ENVELOPE CLASS **************/
typedef enum {
    ENVELOPE_ATTACK = 0,
    ENVELOPE_DECAY,
    ENVELOPE_SUSTAIN,
    ENVELOPE_RELEASE,
} envState;

template<class T> struct envStatus{
    T currentLevel;
    envState state;
};

template struct envStatus<q31>;
template struct envStatus<q16>;

template<class T> class Envelope : public Modulator<T> {
public:
    Envelope();
    ~Envelope() {}

    T getOutput(envStatus<T> *status);
        /* Calculate the output based on the passed status
         * update the status.
         */

    typedef struct {
        T level;
        T inc;
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
    q31 getOutput(envStatus<q31> *statuses);

    typedef struct {
       Modulator<q31> *mod;
       Envelope<q31> env;
    } modSlot;

    Modulator<q16> *carrier;
    modSlot mods[OP_MAX_INPUTS];
private:
};

class Voice;

/************* ALGORITHM CLASS **************/
class Algorithm : public Modulator<q31>{
public:
    Algorithm(Operator **operators, uint8_t numOperators);
    ~Algorithm() {}

    /* runs all operators for a given voice, return mixed output */
    q31 getOutput(Voice *voice) {
        /* for each operator:
         *      getOutput(voice->envStatuses[i]) ... this calculates dependant mods (including operators)
         *
         * sum all and return
         */
    }
private:
    Operator **ops;
    uint8_t numOps;
};

/************* VOICE CLASS **************/
class Voice : public Modulator<q16>{
public:
    Voice(Algorithm *algo);
    ~Voice() {}

    q16 frequency;
    q16 getOutput() { return frequency; }

    q31 play() { return algorithm->getOutput(this); }

    envStatus<q31> envStatuses[FM_MAX_OPERATORS][OP_MAX_INPUTS];
private:
    Algorithm *algorithm;
};


#endif /* AUDIOFX_FM_H_ */
