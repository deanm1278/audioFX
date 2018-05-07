/**************************************
 * This example shows how to do an FIR lowpass filter
 */

#include "audioFX.h"
#include "adau17x1.h"
#include "delay.h"

adau17x1 iface;

#define NUM_COEFFS 21

RAMB q31 firbuf[AUDIO_BUFSIZE+NUM_COEFFS];
q31 firCoeffs[NUM_COEFFS] = {
	  _F(-0.02010411882885732),
	  _F(-0.05842798004352509),
	  _F(-0.061178403647821976),
	  _F(-0.010939393385338943),
	  _F(0.05125096443534972),
	  _F(0.033220867678947885),
	  _F(-0.05655276971833928),
	  _F(-0.08565500737264514),
	  _F(0.0633795996605449),
	  _F(0.310854403656636),
	  _F(0.4344309124179415),
	  _F(0.310854403656636),
	  _F(0.0633795996605449),
	  _F(-0.08565500737264514),
	  _F(-0.05655276971833928),
	  _F(0.033220867678947885),
	  _F(0.05125096443534972),
	  _F(-0.010939393385338943),
	  _F(-0.061178403647821976),
	  _F(-0.05842798004352509),
	  _F(-0.02010411882885732),
};

struct fir *filter;

RAMB q31 left[AUDIO_BUFSIZE], right[AUDIO_BUFSIZE];

void audioLoop(q31 *data)
{
	deinterleave(data, left, right);
	copy(right, left);

	FIRProcess(filter, left, left);

	//right is unfiltered, left is filtered
	interleave(data, left, right);
}

void setup(){
	filter = initFIR(firbuf, AUDIO_BUFSIZE+NUM_COEFFS, firCoeffs, NUM_COEFFS);

	iface.begin();
	fx.setHook(audioLoop);
	fx.begin();

}

void loop(){
	__asm__ volatile ("IDLE;");
}

