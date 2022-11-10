# Project 2: User-Level Thread Library

## Summary

In this project my partner and I have implemented a user-level thread library 
that allows users access to a multi-threading library containing functions
allowing for thread creation, round-robin thread scheduling, a semaphore-based
thread synchronization system, and being preemptive.

### Approach

My partner and I were able to efficiently and collaboratively complete this 
project by breaking down the assignment into the Phases 1 through 4 as suggested 
by the provided HTML document. Additional resources we used that proved to be 
very helpful were the lecture slides and notes, Geeksforgeeks.org, and the 
relevant GNU manual pages.

## Implementation

The workflow suggested by the HTML document that we followed were the 
following steps:

1. Phase 1: queue API
2. Phase 2: uthread API
3. Phase 3: semaphore API
4. Phase 4: preemption

### Queue API

In order to meet the operational time constraints as well as being memory 
efficient the data structure that we selected for our queue implementation was
a singly-linked list. In order to build our ```Struct Queue``` the data fields
that we kept track of were the head and tail of the linked list, and the length
of the queue which has the number of nodes it contains in it. These 3 data 
fields allowed us to incorporate the rest of the functions provided in the
queue.h header file. The functions provided by our Queue API were 
```queue_create()```, ```queue_destroy()```, ```queue_enqueue()```, 
```queue_dequeue()```, and ```queue_delete()```. As I mentioned previously, an
advantage of using a linked list as the data structure for our queue is how it
is memory efficient due to its use of dynamic memory allocation. This is better
than a data structure such as an array which statically allocates memory since
the size of the queue we needed was unknown and in an array, we may either have 
not enough space for our queue, or too much space and memory blocks in the array
end up not being used. To implement queue creation we allocated memory for the 
queue and then initialized the data field to that of an empty queue. To delete 
a queue we first checked if it was an empty and then freed the allocated memory.
To enqueue a node we allocated memory for the node, and then added it to the 
queue by setting the tail's next node pointer to the node we just added. To 
dequeue a node, we had to obey the FIFO nature of queues and dequeue the oldest
node which we kept track of as the head of the queue. We stored this dequeued
data value in the passed in pointer and then freed the memory. To delete a 
a node with a specific value, we used a while loop that checked if the data
matched and if it did, we had to first perform some housekeeping where 
we set the previous node's next pointer to the value of the node after the 
node to be deleted. This one done in order to maintain the linked nature
of our queue. For all of the functions above, we ensured proper input parameters
by checking at the beginning if non-NULL values were input otherwise it would
return -1. A helpful resource we followed for this phase was Geeksforgeeks.org
which explained the design principles of a linked-list and queue, and included
some pseudocode as to how to implement them and some of their functions. We 
implemented the queue iterate function by traversing through each node of the 
queue and executing the passed in function on the data value of the node.


### Uthread API

To begin our implementation of the UThread API contained in uthread.c, we 
first examined ```private.h```, ```context.c```, and ```uthread.h``` along 
with consulting the Linux man pages about the context functions to help us 
understand how to utilize the functions in ```context.c``` to create contexts 
and switch contexts. We created a ```TCB struct``` which has 3 variables: 
```execContext```, ```stackPointer```, and ```fromSemaphore```. 
```uthread_run``` was our main driver / IDLE thread. It allocated 3 queues, 
for running threads, ready threads, and blocked threads. It then created a 
thread using uthread_create for the initial thread that is passed in as 
argument to ```uthread_run``` and placed it in the readyQueue.
```uthread_create``` fills in the execution context variable execContext and 
stack pointer stackPointer using the ```uthread_ctx_init``` command and 
```uthread_ctx_alloc_stack()```. Finally, ```uthread_run``` calls 
```uthread_yield```, which on a high level removes the thread from the 
runningQueue and switch contexts with the first thread in the readyQueue 
using ```uthread_ctx_switch``` and then activates that thread. It also 
deletes the queues if it determines that there are no threads running. We also
 initialized a global current pointer that would point to the node in 
runningQueue and be returned at every call of ```uthread_current```. Finally, 
after a thread finishes, ```uthread_exit``` deallocates the stack of that thread 
using the given context function and calls ```uthread_yield``` to the next item 
in readyQueue. 

### Semaphore API

In order to start the Semaphore API, we first had to define a ```semaphore 
(sem) struct``` that held the resourceCounts, a count of blocked threads 
enqueued in the blocked threads queue by the specific semaphore, and the TCBs 
of the blocked threads. ```sem_create``` initialized all the variables: 
```resourceCount``` to a user-passed in resource count, ```isBlocked``` to 0, 
and ```waitlist queue``` to a empty queue using queue_create. ```sem_down``` 
was created for two situations: if the resourceCount of the semaphore was 0, 
the current threads TCB was placed into our uthread blocked queue using 
```uthread_block``` and stored in the semaphores waitlist. If the 
```resourceCount``` was larger than 0, the function would decrement 
```resourceCount``` and continue executing the current thread by allowing it 
to run. ```sem_up``` incremented the resourceCount and released the latest 
thread from the sem's OWN waitlist by calling ```uthread_unblock```, which 
removed a node from the waitlist and passing it into ```queue_delete```, which 
would find and delete the thread TCB stored in the blocked queue and then pass 
that threadTCB into the ```readyQueue``` to be executed. The 
```fromSemaphore``` variable in the TCB was set to 1 every time 
```uthread_block``` was called, so that ```uthread_yield``` would not enqueue a 
blocked TCB from a semaphore into the run/readyqueue. ```uthread_block``` was a 
function that would block the thread and place it in the blocked queue and then 
yield to the next thread. 

### Preemption

The purpose of preemption is so that our thread library can schedule threads by 
itself even if ```uthread_yield()``` or the semaphore block is never called. 
In order to implement this my partner and I closely followed the GNU manual 
pages linked in the HTML document as well as 2 steps it proposed. The 2 steps 
were also easy to follow as they were essentially the implementation of 2 sets
of functions, namely the ```preempt_start()```, ```preempt_stop()``` and
```preempt_enable()```, ```preempt_disable()``` functions. The first step 
encompassed setting up a signal handler that calls ```uthread_yield()``` 
whenever it receives a signal of the type SIGVTALRM and then configuring 
a timer that would send the alarm signal at a rate of 100hz or 100 times a 
second. To implement the timer, what we did was set the microsecond parameter 
of the timer to 10000 which is 0.01 seconds. Doing so made it so an alarm gets 
sent 100 times in 1 second. To implement the preempt stop we just needed to
undo the code and preempt start and reset the signal handler to its default
state. The second step was implementing the preemption enable and disable 
functions which we used in uthread.c to ensure that when data structures
were being modified, the thread would not be interrupted since this step
needs to be atomic. In both functions, we did this by setting up a signal mask
for blocking and unblocking the SIGVTALRM signal.

### Testing
Testing on each phase of this project was done by adding the finished C source
code to the libuthread library, updating the makefile so it included the new
source code, in the apps directory there contained testers which were compiled
into an executable that would test the output of our code. 
