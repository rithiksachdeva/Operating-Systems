# Target library
lib := libuthread.a
CFLAGS = -Wall -Werror -Wextra

all: preempt.o context.o queue.o uthread.o sem.o $(lib)

$(lib): 
	ar rcs libuthread.a preempt.o context.o queue.o uthread.o sem.o

queue.o: queue.c
	gcc -g -od -c $^ $(CFLAGS) -o $@

uthread.o: uthread.c
	gcc -g -od -c $^ $(CFLAGS) -o $@

context.o: context.c
	gcc -g -od -c $^ $(CFLAGS) -o $@

sem.o: sem.c
	gcc -g -od -c $^ $(CFLAGS) -o $@

preempt.o: preempt.c
	gcc -g -od -c $^ $(CFLAGS) -o $@
clean:
	rm *.o libuthread.a
