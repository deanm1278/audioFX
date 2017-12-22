#include "audioFX.h"
#include "audioRingBuf.h"
#include "scheduler.h"

//create the fx object
AudioFX fx;

//create a process scheduler
Scheduler sch;

//this will hold past input data
AudioRingBuf buf(256, &fx);

volatile int N;

#define MAX_PRIO 6

/* process block of size N. This is
 * done using the overlap-save method.
 */
void NBlock(){
}

/* process block of size 2N */
void N2Block(){
	/* TODO: Queue the MDMA job for the following
	 * 2N block at the end of this one
	 */
}

/* process block of size 4N */
void N4Block(){
	/* TODO: Queue the MDMA job for the following
	 * 4N block at the end of this one
	 */
}

/*
 * This is called from interrupt context when
 * a buffer is ready
 */
void audioHook(q31 *data)
{
	N++;
	//process a block of size N every time
	sch.addTask(NBlock, MAX_PRIO);


	if(N%2 == 0){
		sch.addTask(N2Block, MAX_PRIO - 1);
	}
	if(N%4 == 0){
		sch.addTask(N4Block, MAX_PRIO - 2);
	}
}

void setup() {
  //begin the scheduler
  sch.begin();
  N = 0;

  fx.begin();
  fx.setHook(audioHook);
}

void loop() {
	/*
	 * Since we are using the interrupt-driven implementation,
	 * nothing happens in here.
	 */
}
