/*
 File: scheduler.C
 
 Author: Sabyasachi Gupta
 Date  : 3/28/2019
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */
#ifndef NULL
#define NULL 0L
#endif
//added NULL defination
/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/
// NULL thread creation with infinite while loop
void fun() {
	while(1);
}

Scheduler::Scheduler() {
  //assert(false);
  q_size = 0; //defining queue size as zero
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  //assert(false);
  //disabling interrupts for yield
  if(Machine::interrupts_enabled())
	  Machine::disable_interrupts();
  
  
  if (q_size != 0) { //for non zero q size
     q_size--;
     Thread* crt_thrd= rdy_q.del(); //retrieving one thread
     Thread::dispatch_to(crt_thrd); //dispatching to above mentioned thread
  } else { //calling null thread for empty q
      Console::puts("Empty Queue..Creating Null thread..\n");
	  char * stack = new char[1024];
      Thread* thrd = new Thread(fun, stack, 1024);
	  Thread::dispatch_to(thrd); //dispatching to null thread
  }
}

void Scheduler::resume(Thread * _thread) {
  //assert(false);
  rdy_q.insert(_thread); //adding to ready q at bottom
  q_size++; // increasing q size
  
}

void Scheduler::add(Thread * _thread) {
  //assert(false);
  rdy_q.insert(_thread); //adding to ready q at bottom
  q_size++; // increasing q size
}

void Scheduler::terminate(Thread * _thread) {
  //assert(false);
  //iterating over each node in ready q
  for (int i = 0; i < q_size; i++) {
      Thread * srt_thrd = rdy_q.del(); //retreiving the first node from q
      if (_thread->ThreadId() == srt_thrd->ThreadId()) {
          q_size--; //reducing size if thread id match
      } else {
           rdy_q.insert(srt_thrd); // putting the thread back in at bottom
      }
  }
}
