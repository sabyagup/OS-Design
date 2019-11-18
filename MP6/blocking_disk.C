/*
     File        : blocking_disk.c

     Author      : Sabyasachi Gupta
     Modified    : 4/14/2019

     Description : blocking disk implementation

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H" 
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"
//add SYSTEM_SCHEDULER using extern
extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

//method to add the blocked thread in scheduler queue
void BlockingDisk::wait_until_ready() {
	while(!SimpleDisk::is_ready()) { //checking for blocked thread
		SYSTEM_SCHEDULER->resume(Thread::CurrentThread()); //calling resume on current thread
		SYSTEM_SCHEDULER->yield(); //yielding the resource
	}
}


void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::read(_block_no, _buf);
}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  SimpleDisk::write(_block_no, _buf);
}


/*bool BlockingDisk::is_ready() {
    return SimpleDisk::is_ready();
}*/