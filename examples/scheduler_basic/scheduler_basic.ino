/*  This is a basic example of how to use the task scheduler with the
 *  AudioFX library. This approach can be useful when we need to process blocks of
 *  data in the background and want to keep processor demand uniform.
 * 
 *  This simple example just gains both channels down by 50%.
 */

#include "audioFX.h"
#include "scheduler.h"

AudioFX fx;
Scheduler sch;

//this will hold interleaved data
int32_t *currentBuf;

void setGain(){
  for(int i=0; i<AUDIO_BUFSIZE; i++){
    //Left
    *currentBuf = FRACMUL(*currentBuf, .5);
    currentBuf++;

    //Right
    *currentBuf = FRACMUL(*currentBuf, .5);
    currentBuf++;
  }
}

//This will run in interrupt context
void audioHook(int32_t *data)
{
  //save the pointer to the new data
  currentBuf = data;

  /* Add a task at the highest priority.
   * This task will begin as soon as we return from the buffer
   * received interrupt.
   */
  sch.addTask(setGain, SCHEDULER_MAX_PRIO);
}

// the setup function runs once when you press reset or power the board
void setup() {
  fx.begin();
  sch.begin();

  //set the function to be called when a buffer is ready
  fx.setHook(audioHook);

  /* loop() will never exit, and just puts the processor in
   * idle state waiting for an interrupt.
   * Set this as the lowest priority task.
   */
  sch.addTask(loop, 0);
}

void loop() {
  while(1) __asm__ ("IDLE;");
}
