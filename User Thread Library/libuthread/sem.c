#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "private.h"

//Create semaphore that has the amount of resources it has available (resourceCount), the amount of threads its blocked (isBlocked), and the TCB of the blocked threads (waitlist)
struct semaphore {
	int resourceCount;
	int isBlocked;
	queue_t waitlist;
};

sem_t sem_create(size_t count)
{
	//create memory for semaphore and return null if it doesnt work. 
	sem_t semaphore1 = (sem_t)malloc(sizeof(struct semaphore));
	if (semaphore1 == NULL){
		return NULL;
	}
	//set resourceCount to count and allocate memory for the waitlist queue. set isblocked to 0 and return pointer to semaphore
	semaphore1->resourceCount = count;
	semaphore1->waitlist = malloc(sizeof(queue_t));
	semaphore1->isBlocked = 0;
	return semaphore1;
}

int sem_destroy(sem_t sem)
{
	//assuming semaphore is not still in use or had blocked thread, free waitlist and then the semaphore. 
	if (sem == NULL || sem->isBlocked > 0){
		return(-1);
	}
	free(sem->waitlist);
	free(sem);
	return(0);
}

int sem_down(sem_t sem)
{
		/* Step 1: Check if Sem is null
		 * Step 2: Add the TCB into waitlist
		 * Step 3: increment isblocked
		 * Step 4: place in blocked queue using uthread block
		 * Step 2.2: if resource count > 0, just decrement resourceCount and continue executing thread
		 */
	if(sem == NULL){
		return -1;
	}

	if(sem->resourceCount == 0){
		queue_enqueue(sem->waitlist, uthread_current());
		sem->isBlocked++;	
		uthread_block();
	}else{
		//if I have a valid resource, I just continue executing rather than yielding. 
		sem->resourceCount--;
	}
	return 0;
}

int sem_up(sem_t sem)
{
	/* Step 1: Check if sem is Null
	 * Step 2: increment resource Count
	 * Step 3: if there are blocked threads on the semaphore, pull the oldest out from waitlist and use uthread_unblock to delete it from blocked queue and place in readyqueue
	 * Step 4: decrement isblocked
	 */
	if(sem == NULL){
		return -1;
	}
	
	sem->resourceCount++;

	if (queue_length(sem->waitlist) != 0){
		struct uthread_tcb* unblockData;
		queue_dequeue(sem->waitlist, (void *)&unblockData);
		uthread_unblock(unblockData);
		sem->isBlocked--;
	}
	return 0;
}

