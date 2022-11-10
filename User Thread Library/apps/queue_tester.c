#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>

#define TEST_ASSERT(assert)				\
do {									\
	printf("ASSERT: " #assert " ... ");	\
	if (assert) {						\
		printf("PASS\n");				\
	} else	{							\
		printf("FAIL\n");				\
		exit(1);						\
	}									\
} while(0)

/* Create */
void test_create(void)
{
	fprintf(stderr, "*** TEST create ***\n");

	TEST_ASSERT(queue_create() != NULL);
}

/* Enqueue/Dequeue simple */
void test_queue_simple(void)
{
	int data = 3;
	int *ptr;
	queue_t q;

	fprintf(stderr, "*** TEST queue_simple ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_dequeue(q, (void**)&ptr);
	TEST_ASSERT(ptr == &data);
}

/* Test Queue Length  */
void test_queue_length(void)
{
	int data = 10;
	int data2 = 20;
	int data3 = 30;
	int data4 = 40;
	int length;
	queue_t q;

	fprintf(stderr, "*** TEST queue_length ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);
	length = queue_length(q);

	TEST_ASSERT(length == 4);
}

/* Enqueuing NULL */
void test_queue_null(void)
{
	queue_t q;

	fprintf(stderr, "*** TEST queue NULL ***\n");

	q = queue_create();
	TEST_ASSERT(queue_enqueue(q,NULL) == -1);
}

/* Dequeuing NULL */
void test_dequeue_null(void)
{
	queue_t q;

	fprintf(stderr, "*** TEST dequeue NULL ***\n");

	q = queue_create();
	TEST_ASSERT(queue_dequeue(q,NULL) == -1);
}

/*Testing enqueue and dequeue*/
void test_queue_enqdeq(void)
{
	int data = 10;
	int data2 = 20;
	int data3 = 30;
	int data4 = 40;
	int *ptr;
	queue_t q;

	fprintf(stderr, "*** TEST enqueing and dequeuing ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);
	queue_dequeue(q, (void**)&ptr);
	queue_dequeue(q, (void**)&ptr);

	TEST_ASSERT(ptr == &data2);
}

/*Test queueing and deletion*/
void test_queue_delete(void)
{
	int data = 10;
	int data2 = 20;
	int data3 = 30;
	int data4 = 40;
	queue_t q;
	int length;

	fprintf(stderr, "*** TEST deletion ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);
	queue_delete(q, &data3);
	queue_delete(q, &data2);
	length = queue_length(q);

	TEST_ASSERT(length == 2);
}

/*Test deletion not found*/
void test_queue_delfind(void)
{
	int data = 10;
	int data2 = 20;
	int data3 = 30;
	int data4 = 40;
	int data5 = 50;
	queue_t q;

	fprintf(stderr, "*** TEST deletion if not found***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);

	TEST_ASSERT(queue_delete(q,&data5) == -1);
}

/*Testing deletion and dequeue*/
void test_queue_deldeq(void)
{
	int data = 10;
	int data2 = 20;
	int data3 = 30;
	int data4 = 40;
	queue_t q;
	int *ptr;

	fprintf(stderr, "*** TEST deletion ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_enqueue(q, &data2);
	queue_enqueue(q, &data3);
	queue_enqueue(q, &data4);
	queue_delete(q, &data2);
	queue_dequeue(q, (void**)&ptr);
	queue_dequeue(q, (void**)&ptr);
	
	TEST_ASSERT(ptr == &data3);
}

/*Testing queue destroy*/
void test_queue_destroy(void)
{
	int data = 10;
	queue_t q;
	int *ptr;

	fprintf(stderr, "*** TEST queue destroy ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_dequeue(q, (void**)&ptr);
	
	TEST_ASSERT(queue_destroy(q) == 0);
}

/*Testing queue destroy null*/
void test_queue_destnull(void)
{
	fprintf(stderr, "*** TEST queue destroy NULL ***\n");
	queue_t q;
	q = NULL;
	TEST_ASSERT(queue_destroy(q) == -1);
}

int main(void)
{
	test_create();
	test_queue_simple();
	test_queue_null();
	test_dequeue_null();
	test_queue_length();
	test_queue_enqdeq();
	test_queue_delete();
	test_queue_delfind();
	test_queue_deldeq();
	test_queue_destroy();
	test_queue_destnull();
	return 0;
}
