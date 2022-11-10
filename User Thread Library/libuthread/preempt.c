#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100

/*Set the sighandler to be uthread_yield so when the alarm goes off the thread yields*/
void yield_handler (int signum) {
	if (signum == SIGVTALRM) {
		uthread_yield();
	}
}

void preempt_disable(void)
{
	/*Disable preempt for accessing critical areas of data structure*/
	sigset_t block_preempt;
	sigemptyset(&block_preempt);
	sigaddset(&block_preempt, SIGVTALRM);
	sigprocmask(SIG_BLOCK, &block_preempt, NULL); //The signal SIGVTALRM should be blocked
}

void preempt_enable(void)
{
	/*After properly managing data structures allow preempt again*/
	sigset_t unblock_preempt;
	sigemptyset(&unblock_preempt);
	sigaddset(&unblock_preempt, SIGVTALRM);
	sigprocmask(SIG_UNBLOCK, &unblock_preempt, NULL); //The signal SIGVTALRM should not be blocked
}


/*1. Install a signal handler that receives alarm signals (of type SIGVTALRM)*/
/*2. Configure a timer which will fire an alarm a hundred times per second*/
void preempt_start(bool preempt)
{
	/*Initializing the signal handler so that when an alaram signal is received,
	the handler with uthread_yield is called*/
	/*Part 1*/
	if (preempt == true) {
		/*Part 1*/
		struct sigaction sact;
		sigemptyset(&sact.sa_mask);
		sact.sa_flags = 0;
		sact.sa_handler = yield_handler;
		sigaction(SIGVTALRM, &sact, NULL);

		/*Part 2*/
		struct itimerval timer;
		timer.it_value.tv_usec = (1/HZ) * 1000000;
		timer.it_value.tv_sec = 1/HZ; 
		timer.it_interval = timer.it_value;
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
	}
}

/*Restore previous signal action and timer values */
void preempt_stop(void)
{
	/*Undoing Part 1 by setting the signal back to default*/
	struct sigaction sact;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = yield_handler;
	sigaction(SIGVTALRM, &sact, NULL);

	/*Resetting the timer values and stopping the alarm*/
	struct itimerval timer;
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0; 
	timer.it_interval = timer.it_value;
	setitimer(ITIMER_VIRTUAL, &timer, NULL);
	
	/*Undoing Part 2 by restoring the timer values*/
}

