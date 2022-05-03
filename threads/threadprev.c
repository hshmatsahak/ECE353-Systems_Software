#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

#define READY 0
#define RUNNING 1
#define EMPTY 2
#define EXITED 3

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

/* This is the thread control block */
struct thread { 
	Tid id; 
	int state;
	ucontext_t context;

	int prev;
	int next;

	int yield_count;

	void *sp;
};

struct thread threads[THREAD_MAX_THREADS];
int head;
int tail;

struct thread running_thread;

// void pht(){
// 	int id = head;
// 	printf("--------------\n");
// 	while (id != -1){
// 		printf("%d ", id);
// 		id = threads[id].next;
// 	}
// 	printf("--------------\n");
// 	printf("current is %d\n", thread_id());
// 	printf("head is %d", head);
// 	printf("tail is %d", tail);
// 	for (int i = 0; i < THREAD_MAX_THREADS; i += 1) {
// 		printf("%d", threads[i].state);
// 	}
// }

// void meminfo(){
// 	printf("Hi\n\n\n\n");
// 	for (int i = 0; i < THREAD_MAX_THREADS; i += 1) {
// 		if (threads[i].sp != NULL){
// 			printf("%d isnt null", i);
// 		}
// 	}
// 	printf("Hi\n\n\n\n");
// }

void
thread_init(void)
{
	// initialize ready queue
	for (int i = 0; i < THREAD_MAX_THREADS; i++){
		threads[i].id = i;
		threads[i].state = EMPTY;
		threads[i].yield_count = 0;
	}
	threads[0].state = RUNNING; 
	head = -1;
	tail = -1;

	// initialize running queue
	running_thread.state = RUNNING;
}

Tid
thread_id()
{
	return running_thread.id;
}

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	thread_main(arg); // call thread_main() function with arg
	thread_exit();
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	Tid thread_id = -1;
	for (int i = 0; i < THREAD_MAX_THREADS; i++){
		if (threads[i].state == EMPTY){
			thread_id = i;
			break;
		}
	}
	if (thread_id == -1){
		return THREAD_NOMORE;
	}

	threads[thread_id].id = thread_id;
	threads[thread_id].state = READY;
	threads[thread_id].yield_count = 0;
	getcontext(&threads[thread_id].context);
	threads[thread_id].context.uc_mcontext.gregs[REG_RIP] = (long long int) &thread_stub;
	void *sp = (void *) malloc(THREAD_MIN_STACK+16);
	if (sp == NULL){
		return THREAD_NOMEMORY;
	}
	threads[thread_id].sp = sp;
	threads[thread_id].context.uc_mcontext.gregs[REG_RSP] = (long long int) (sp + THREAD_MIN_STACK + 8);
	threads[thread_id].context.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
	threads[thread_id].context.uc_mcontext.gregs[REG_RSI] = (long long int)parg;	

	if (head == -1){
		head = threads[thread_id].id;
		tail = threads[thread_id].id;
		threads[thread_id].prev = -1;
		threads[thread_id].next = -1;
	}else{
		threads[thread_id].prev = tail;
		threads[thread_id].next = -1;
		threads[tail].next = threads[thread_id].id;
		tail = threads[thread_id].id;
	}
	
	return thread_id;
}

Tid
thread_yield(Tid want_tid)
{
	int id = want_tid; 
 
	if (want_tid == THREAD_SELF || want_tid == thread_id()){
		return thread_id();
	}

	if (want_tid == THREAD_ANY){
		id = head;
		if (id == -1){
			return THREAD_NONE;
		}
	} 
	else if (want_tid < 0 || want_tid >= THREAD_MAX_THREADS || threads[want_tid].state != READY){
		return THREAD_INVALID;
	}

	if (running_thread.state == RUNNING){
		threads[thread_id()].state = READY;
		threads[thread_id()].prev = tail;
		threads[tail].next = thread_id();
		tail = thread_id();
	}
	else{
		threads[running_thread.id].yield_count = 0;
	}
	getcontext(&threads[thread_id()].context);

	if (threads[running_thread.id].yield_count == 0){
		threads[running_thread.id].yield_count = 1;
		running_thread.yield_count = 1;

		threads[id].state = RUNNING;
		if (head == id && tail == id){
			head = -1;
			tail = -1;
			threads[id].prev = -1;
			threads[id].next = -1;
		} else if (head == id){
			head = threads[id].next;
			threads[head].prev = -1;
			threads[id].next = -1;
		} else if (tail == id){
			tail = threads[id].prev;
			threads[tail].next = -1;
			threads[id].prev = tail;
		} else{
			threads[threads[id].prev].next = threads[id].next;
			threads[threads[id].next].prev = threads[id].prev;
			threads[id].prev = -1;
			threads[id].next = -1;
		}
		running_thread = threads[id];
		setcontext(&running_thread.context);
	}
		
	running_thread.yield_count = 0;
	threads[running_thread.id].yield_count = 0;

	for (int i = 0; i < THREAD_MAX_THREADS; i++){
		if (threads[i].state == EMPTY){
			free(threads[i].sp);
			threads[i].sp = NULL;
		}
	}

	return id;
}

// This function ensures that the current thread does not run after this call, i.e., this function should never return. 
// If there are other threads in the system, one of them should be run. If there are no other threads (this is the last thread invoking thread_exit),
// then the program should exit. A thread that is created later should be able to reuse this thread's identifier, but only after this thread has been destroyed.
void
thread_exit()
{
	threads[thread_id()].state = EMPTY; 
	running_thread.state = EMPTY; 
 
	if (thread_yield(THREAD_ANY) == THREAD_NONE){
		exit(0);
	}
}

// This function kills another thread whose identifier is tid. The tid can be the identifier of any available thread. 
// The killed thread should not run any further and the calling thread continues to execute.
// Upon success, this function returns the identifier of the thread that was killed. Upon failure, it returns the following:
// THREAD_INVALID: alerts the caller that the identifier tid does not correspond to a valid thread, or is the current thread.
Tid
thread_kill(Tid tid)
{
	if (tid == thread_id() || tid < 0 || tid >= THREAD_MAX_THREADS || threads[tid].state == EMPTY ){
		return THREAD_INVALID;
	}
	
	threads[tid].context.uc_mcontext.gregs[REG_RIP] = (long long int) thread_exit;
	return tid;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
