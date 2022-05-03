#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

#define READY 0
#define RUNNING 1
#define EMPTY 2
#define WAITING 3

/* This is the wait queue structure */
struct wait_queue {
	struct queue* wait_q;
};

/* This is the thread control block */
struct thread { 
	Tid id; 
	ucontext_t context;
	int yield_count;
	void *sp;
};

struct queue_node {
    struct thread thread_block;
    struct queue_node* next;
};

struct queue {
    struct queue_node *head;
    struct queue_node *tail;
};

void init_node(struct queue_node* qn, struct thread tb){
	qn->thread_block = tb;
	qn->next = NULL;
}

void queue_init(struct queue *q){
    q->head = NULL;
    q->tail = NULL;
}

void enqueue(struct queue* q, struct queue_node* node){
	node->next = NULL;
    if (q->head==NULL){
        q->head = node;
        q->tail = node;
    } else{
        (q->tail)->next = node;
        q->tail = node;
    }
}

struct queue_node* deque(struct queue* q, Tid tid){
	struct queue_node* curr = q->head;

	if (curr == NULL){
		return NULL;
	}

	if (curr->thread_block.id == tid || tid == THREAD_ANY){
		q->head = q->head->next;
		if (q->tail == curr){
			q->tail = NULL;
		}
		return curr;
	}

	while(curr->next != NULL && curr->next->thread_block.id != tid){
		curr = curr->next;
	}
	if (curr->next == NULL){
		return NULL;
	}
	struct queue_node* tmp = curr->next;
	if (curr->next == q->tail){
		curr->next = NULL;
		q->tail = curr;
	}
	else{
		curr->next = tmp->next;
	}
	return tmp;	
}

struct queue_node* dequeHead(struct queue* q){
    if (q == NULL || q->head == NULL){
        return NULL;
    }
	return deque(q, q->head->thread_block.id);
}

void deleteAll(struct queue* q){
	struct queue_node* curr = q->head;
	struct queue_node* nxt;
	while(curr!=NULL){
		nxt = curr->next;
		free(curr->thread_block.sp);
		free(curr);
		curr = nxt;
	}
	q->head = NULL;
	q->tail = NULL;
}

struct queue_node* findnode(struct queue* q, Tid tid){
	if (q->head == NULL){
		return NULL;
	}
	struct queue_node* curr = q->head;
	while (curr!=NULL && curr->thread_block.id != tid){
		curr = curr->next;
	}
	if (curr==NULL){
		return NULL;
	}
	return curr;
}

struct queue *running_q;
struct queue *ready_q;
struct queue *exit_q;

int status[THREAD_MAX_THREADS];
struct queue_node* nodes[THREAD_MAX_THREADS]; 
struct wait_queue* thread_wait_queues[THREAD_MAX_THREADS];

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void
thread_init(void)
{
	interrupts_off();

    running_q = (struct queue*) malloc(sizeof(struct queue));
    ready_q = (struct queue*) malloc(sizeof(struct queue));
	exit_q = (struct queue*) malloc(sizeof(struct queue));

    queue_init(running_q);
	queue_init(ready_q);
	queue_init(exit_q);

    struct thread first_thread;
    first_thread.id = 0;
    first_thread.yield_count = 0;
	struct queue_node* first_node = (struct queue_node*) malloc(sizeof(struct queue_node));
	init_node(first_node, first_thread);
    enqueue(running_q, first_node);

    status[0] = RUNNING;
	for (int i = 1; i < THREAD_MAX_THREADS; i++){
		status[i] = EMPTY;
	}

    nodes[0] = first_node;
    for (int i = 1; i < THREAD_MAX_THREADS; i++){
		nodes[i] = NULL;
        thread_wait_queues[i] = wait_queue_create();
	}

	interrupts_on();
}

Tid
thread_id()
{
	return (running_q->head)->thread_block.id;
}

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	interrupts_on();
	thread_main(arg); // call thread_main() function with arg
	thread_exit();
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	interrupts_off();
	
	Tid thread_id = -1;
	for (int i = 0; i < THREAD_MAX_THREADS; i++){
		if (status[i] == EMPTY){
			thread_id = i;
			break;
		}
	}
	if (thread_id == -1){
		interrupts_on();
		return THREAD_NOMORE;
	}

    struct thread new_threadblock;
	new_threadblock.id = thread_id;
	new_threadblock.yield_count = 0;
	getcontext(&new_threadblock.context);
	new_threadblock.context.uc_mcontext.gregs[REG_RIP] = (long long int) &thread_stub;
	void *sp = (void *) malloc(THREAD_MIN_STACK+16);
	if (sp == NULL){
		interrupts_on();
		return THREAD_NOMEMORY;
	}
	new_threadblock.sp = sp;
	new_threadblock.context.uc_mcontext.gregs[REG_RSP] = (long long int) (sp + THREAD_MIN_STACK + 8);
	new_threadblock.context.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
	new_threadblock.context.uc_mcontext.gregs[REG_RSI] = (long long int)parg;

	status[new_threadblock.id] = READY;

	struct queue_node* new_node = (struct queue_node*) malloc(sizeof(struct queue_node));
	init_node(new_node, new_threadblock);
	enqueue(ready_q, new_node);
    nodes[thread_id] = new_node;
	interrupts_on();
    
	return thread_id;
}

Tid
thread_yield(Tid want_tid)
{
	interrupts_off();

	int id = want_tid; 
 
	if (want_tid == THREAD_SELF || want_tid == thread_id()){
		interrupts_on();
		return thread_id();
	}

	if (want_tid == THREAD_ANY){
		if (ready_q->head==NULL){
			interrupts_on();
			return THREAD_NONE;
		}
		id = ready_q->head->thread_block.id;
	} 
	else if (want_tid < 0 || want_tid >= THREAD_MAX_THREADS || status[want_tid] != READY){
		interrupts_on();
		return THREAD_INVALID;
	}

	if (status[thread_id()] == RUNNING){
		status[thread_id()] = READY;
		enqueue(ready_q, running_q->head);
	}
	else if (status[thread_id()] != WAITING){
		running_q->head->thread_block.yield_count = 0;
	}
	getcontext(&running_q->head->thread_block.context);

	if (running_q->head->thread_block.yield_count == 0){
		running_q->head->thread_block.yield_count = 1;

		status[id] = RUNNING;
		struct queue_node* new_node = deque(ready_q, id);
		running_q->head = new_node;
		running_q->tail = new_node;
		setcontext(&running_q->head->thread_block.context);
	}
	
	running_q->head->thread_block.yield_count = 0;

	deleteAll(exit_q);

	interrupts_on();

	return id;
}

// This function ensures that the current thread does not run after this call, i.e., this function should never return. 
// If there are other threads in the system, one of them should be run. If there are no other threads (this is the last thread invoking thread_exit),
// then the program should exit. A thread that is created later should be able to reuse this thread's identifier, but only after this thread has been destroyed.
void
thread_exit()
{
	interrupts_off();

    thread_wakeup(thread_wait_queues[thread_id()], 1);

	status[thread_id()] = EMPTY;

	enqueue(exit_q, running_q->head);

	// interrupts_on();

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
	interrupts_off();
	if (tid == thread_id() || tid < 0 || tid >= THREAD_MAX_THREADS || status[tid] == EMPTY ){
		interrupts_on();
		return THREAD_INVALID;
	}
	
    nodes[tid]->thread_block.context.uc_mcontext.gregs[REG_RIP] = (long long int) thread_exit;
	interrupts_on();
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

	wq = (struct wait_queue*) malloc(sizeof(struct wait_queue));
	assert(wq);

	wq->wait_q = (struct queue*) malloc(sizeof(struct queue));
	queue_init(wq->wait_q);

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	deleteAll(wq->wait_q);
	free(wq);
}

// This function suspends the caller and then runs some other thread. The calling thread is put in a wait queue passed as a parameter to the function. 
// The wait_queue data structure is similar to the run queue, but there can be many wait queues in the system, one per type of event or condition. 
// Upon success, this function returns the identifier of the thread that took control as a result of the function call. 
// The calling thread does not see this result until it runs later. Upon failure, the calling thread continues running, and returns one of these constants:
Tid
thread_sleep(struct wait_queue *queue)
{
    interrupts_off();
	if (queue == NULL){
        interrupts_on();
		return THREAD_INVALID;
	}
	if (ready_q->head == NULL){
        interrupts_on();
		return THREAD_NONE;
	}
	enqueue(queue->wait_q, running_q->head);
	status[thread_id()] = WAITING;
	Tid id = thread_yield(THREAD_ANY);
    interrupts_on();
	return id;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
    interrupts_off();
	if (queue == NULL || queue->wait_q->head==NULL){
        interrupts_on();
		return 0;
	}
	if (all == 0){
		struct queue_node* woken_node = dequeHead(queue->wait_q);
        status[woken_node->thread_block.id] = READY;
		enqueue(ready_q, woken_node);
        interrupts_on();
		return 1;
	}
	else if (all == 1){
		int size = 0;
		struct queue_node* woken_node = dequeHead(queue->wait_q);
		while (woken_node != NULL){
			size += 1;
            status[woken_node->thread_block.id] = READY;
			enqueue(ready_q, woken_node);
			woken_node = dequeHead(queue->wait_q);
		}
        interrupts_on();
		return size;
	}
	else{
        interrupts_on();
		return 0;
	}
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
    if (tid < 0 || tid >= THREAD_MAX_THREADS || tid == thread_id() || status[tid] == EMPTY){
        return THREAD_INVALID;
    }
	thread_sleep(thread_wait_queues[tid]);
    return tid;
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
