#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

//Create a struct that is a definition of a node that stores the data and the pointer to the next node.
struct node {
	void *data;
	struct node *next;
};

//create a queue struct which has a head and tail pointer and a length of queue attribute
struct queue {
	struct node *head;
	struct node *tail;
	int length;
};

//queue_create creats the queue by first allocating space on queue, and then setting the head and tail to NULL. Length is initialized to 0
queue_t queue_create(void)
{
	/*When we creating a queue we allocate memory for it and set the parameters to that of
	an empty queue*/
	queue_t queue1 = (queue_t)malloc(sizeof(struct queue));
	if (queue1 == NULL) {
		return NULL;
	}
	queue1->head = NULL;
	queue1->tail = NULL;
	queue1->length = 0;
	return queue1;
}

//Destroys the queue by freeing it. if length>0 or queue is null, returns -1
int queue_destroy(queue_t queue)
{
	if (queue == NULL) {
		return -1;
	}
	else if (queue->length != 0) {
		return -1;
	}
	free(queue);
	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	/*Check for valid parameters*/
	if (queue == NULL) {
		return -1;
	}
	else if (data == NULL) {
		return -1;
	}
	/*Create the node to insert into the end of the queue*/
	struct node *qnode = (struct node*)malloc(sizeof(struct node));
	if (qnode == NULL) {
		return -1;
	}
	qnode->data = data;
	qnode->next = NULL;

	/*Insert qnode into queue, queue might be empty or non-empty where we need to handle both cases seperately*/
	if (queue->length == 0) {
		queue->head = qnode;
		queue->tail = qnode;
	}
	else {
		queue->tail->next = qnode;
		queue->tail = qnode;
	}
	queue->length++;
	return 0;
}


int queue_dequeue(queue_t queue, void **data)
{
	/*Check for valid parameters*/
	if (queue == NULL) {
		return -1;
	}
	else if (data == NULL) {
		return -1;
	}
	else if (queue->length == 0) {
		return -1;
	}
	/*Assign data to the address of data being queue_dequeued*/
	*data = queue->head->data;
	/*Then we need to place the head into a node that will be freed and update the head to the next node*/
	struct node *dequeue = queue->head;
	queue->head = queue->head->next;
	free(dequeue);
	queue->length--;
	/*Decrement queue length and if we have an empty queue then head and tail should both be NULL*/
	if (queue->length == 0) {
		queue->head = NULL;
		queue->tail = NULL;
	}
	return 0;
} 

int queue_delete(queue_t queue, void *data)
{
	/*Check for valid parameters*/
	if (queue == NULL) {
		return -1;
	}
	else if (data == NULL) {
		return -1;
	}
	struct node *delete = queue->head; 
	struct node *prev = delete; //Need to keep track of previous node so we can connect the next node of the one being deleted with previous

	/*Iterate through the queue until we find data or we reach the end of queue*/
	while (delete != NULL && delete->data != data) { //Search through the queue for data and set previous node
		prev = delete;
		delete = delete->next;
	}
	if (delete == NULL) { //if delete equals NULL that means that data was not found otherwise we would have broken out of loop
		return -1;
	}		
	prev->next = delete->next; //delete was the intermediary between previous->next and delete->next so now we must join the two together
	if (prev->next == NULL){
		queue->tail = prev;
	}
	free(delete);
	queue->length--;
	if (queue->length == 0) { 
		queue->head = NULL;
		queue->tail = NULL;
	}
	return 0;
}


int queue_iterate(queue_t queue, queue_func_t func)
{
	//Checking for valid parameters
	if (queue == NULL) {
		return -1;
	}
	else if (func == NULL) {
		return -1;
	}
	/*Create a node that traverses the queue and store the nodes data*/
	struct node *iter = queue->head;
	void *iter_data = iter->data;
	while(iter != NULL) {
		func(queue,iter_data); //apply each function to each data
		if(iter_data == NULL) { //if item was deleted then return -1
			return -1;
		}
		iter = iter->next; //iterated
		iter_data = iter->data; //repoint at iter data var
	}
	return 0;
} 

//Returns length of queue. 
int queue_length(queue_t queue)
{
	if (queue == NULL) {
		return -1;
	}
	return queue->length;
}
