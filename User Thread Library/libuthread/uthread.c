#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

queue_t readyQueue, runQueue, blockedQueue;

struct uthread_tcb {
	uthread_ctx_t execContext; 
	void* stackPointer;
       	int fromSemaphore; 	
};

struct uthread_tcb *uthread_current(void)
{
	// if the queue doesn't have anything running aka current pointer will point to nothing, return NULL
	if (queue_length(runQueue) == 0) {
		return(NULL);
	}
	//establish a currentTCB pointer and dequeue from runqueue to point the currentTCB pointer at the currently running thread
	struct uthread_tcb* currentTCB;
	queue_dequeue(runQueue, (void *)&currentTCB);
	//if the thread has been placed in blocked queue, do not return it to the runQueue
	if(currentTCB->fromSemaphore == 0){
		queue_enqueue(runQueue, currentTCB);
	}
	//return pointer to currently running thread
	return(currentTCB);
}

void uthread_yield(void)
{
	preempt_disable();
	//If there are no threads currently waiting to execute, check the blocked queue. IF that is also empty, destroy all 3 queues. 
	if(queue_length(readyQueue) == 0) {
		if(queue_length(blockedQueue) == 0){
			queue_destroy(readyQueue);
			if(queue_length(runQueue) == 0) {
				queue_destroy(runQueue);
			}
			if(queue_length(blockedQueue) == 0) {
				queue_destroy(blockedQueue);
			}
		//if readyqueue is empty but blocked queue is not, dequeue one from blockedqueue and place in readyqueue
		}else{
			struct uthread_tcb* blocked = uthread_current();
			queue_dequeue(blockedQueue, (void *)blocked);
			queue_enqueue(readyQueue, blocked);
		}
		return;
	}
	//get the currently running threads execution context by dequeuing it from runQueue
	struct uthread_tcb* currentTCB = uthread_current();
	if(currentTCB != NULL){
		queue_dequeue(runQueue,(void *)currentTCB);
	}
	//get the next threads execution context by dequeueing from readyQueue. if there is nothing currently running (currentTCB is NULL) create a empty TCB. 
	//if thread is not related to semaphore aka blocked, enqueue it to readyQueue
	struct uthread_tcb* yieldtoTCB;
	if (currentTCB == NULL) {
		currentTCB = (struct uthread_tcb*)malloc(sizeof(struct uthread_tcb));
	} else if (currentTCB->fromSemaphore != 1) {
		queue_enqueue(readyQueue, currentTCB);
	}
	//dequeue from readyqueue and then enqueue the tcb into run queue and switch contexts to the newly currently running thread. 
	queue_dequeue(readyQueue, (void *)&yieldtoTCB);
	queue_enqueue(runQueue, yieldtoTCB);
	preempt_enable();
	uthread_ctx_switch(&currentTCB->execContext, &yieldtoTCB->execContext);

}

void uthread_exit(void)
{
	//get current threads TCB and deallocate the stack along with deleting the runqueue if there is no currently executing thread. Then yield
	struct uthread_tcb* currentTCB = uthread_current(); 
	if (currentTCB != NULL) {
		queue_delete(runQueue, currentTCB);
	}
	uthread_ctx_destroy_stack(currentTCB->stackPointer);
	uthread_yield();
}

int uthread_create(uthread_func_t func, void *arg)
{
	//create a TCB and then assign the variable TCB stackPointer using context function. 
	//then pass the blank execution context variable from TCB and the stack pointer along with the function and arg from the parameters to create a new execution context within TCB
	//queue TCB in the readyQueue to be executed.
	struct uthread_tcb* TCB = (struct uthread_tcb*)malloc(sizeof(struct uthread_tcb));
	TCB->stackPointer = uthread_ctx_alloc_stack();
	int retval = uthread_ctx_init(&TCB->execContext, TCB->stackPointer, func, arg);
	if(retval == 0){
		queue_enqueue(readyQueue, TCB);
		return 0;
	}else{
		return -1;
	}
}

int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	/*Step 1: Create Ready Queue
	 * Step 2: Create IDLE Thread (uthread_create(
	 *
	 */
	if(preempt){
		preempt_start(preempt);
		preempt_enable();
	}
	//Create readyQueue
	readyQueue = queue_create();
	if (readyQueue == NULL){
		return -1;
	}
	//Create runQueue
	runQueue = queue_create();
	if (runQueue == NULL){
		return -1;
	}
	//Create blockedQueue
	blockedQueue = queue_create();
	if (blockedQueue == NULL){
		return -1;
	}
	//create the initial thread and place it in readyqueue
	int retval = uthread_create(func, arg);
	if (retval == -1){
		return -1;
	}
	//yield from idle thread to new thread. 
	uthread_yield();	
	if(preempt){
		preempt_stop();
		preempt_disable();
	}
	return 0;
}

void uthread_block(void)
{
	//find current TCB and then indicate that it has been blocked by semaphore so it is not enqueued in ready or run queue by uthread_yield. 
	//then enqueue it in blocked queue and uthread yield. 
	struct uthread_tcb* currentTCB = uthread_current();
	currentTCB->fromSemaphore = 1;
	if (currentTCB != NULL) {
		queue_enqueue(blockedQueue, currentTCB);
	}
	uthread_yield();
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	//if uthread is blocked, delete it from the blocked queue, otherwise change fromsemaphore so it can be added to ready queue and then enqueue to readyQueue. Then yield. 
	if(uthread != NULL) {
		queue_delete(blockedQueue, uthread);
		uthread->fromSemaphore = 0;	
		queue_enqueue(readyQueue, uthread);
	}
	uthread_yield();
}

