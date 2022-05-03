#include "request.h"
#include "server_thread.h"
#include "common.h"

#define SIZE 3253
// alternative to caching: don't...

struct cache {
	int size;
	int total_file_size;
	struct Item **table;
	struct LRU_queue* lru_list;
};

struct Item {
	struct file_data* filedata;
	int inuse;
	struct Item* next;			
};

struct LRU_queue {
	struct LRU_node* head;
	struct LRU_node* tail; 
};

struct LRU_node {
	char* filename;
	struct LRU_node* next;
	struct LRU_node* prev;
};

int hash_function(char *word, int size){
	int val = 1;
	int add=0;

	for (int i=0; i<strlen(word); i++)
	{
		add = (int) word[i];
        val = val * 33 + add;
	}
    return abs(val % size);
}

struct LRU_queue* init_lruqueue() {
	struct LRU_queue* q = (struct LRU_queue*) malloc(sizeof(struct LRU_queue));
	q->head = NULL;
	q->tail = NULL;

	return q;
}

struct LRU_node* init_lrunode(char *filename) {
	struct LRU_node* node = (struct LRU_node*) malloc(sizeof(struct LRU_node));
	node->filename = strdup(filename);
	node->next = NULL;
	node->prev = NULL;
	return node;
}

struct Item* createItem(struct file_data* data){
	struct Item* item = (struct Item*) malloc(sizeof(struct Item));
	item->filedata = (struct file_data*) malloc(sizeof(struct file_data));
	item->filedata->file_buf = strdup(data->file_buf);
	item->filedata->file_name = strdup(data->file_name);
	item->filedata->file_size = data->file_size;
	item->inuse = 0;
	item->next = NULL;
	return item;
}

void enqueue(struct LRU_node* enq_node, struct LRU_queue* lruq){
	if (lruq->head == NULL){
		lruq->head = enq_node;
		lruq->tail = enq_node;
		enq_node->prev = NULL;
		enq_node->next = NULL;
	} else{
		(lruq->tail)->next = enq_node;
		enq_node->prev = lruq->tail;
		lruq->tail = enq_node;
		enq_node->next = NULL;
	}
}

struct LRU_node* deque(struct LRU_queue* lruq, char *fn){
	struct LRU_node* tmp = lruq->head;
	if (strcmp(tmp->filename, fn)==0){
		lruq->head = lruq->head->next;
		if (lruq->head == NULL){
			lruq->tail = NULL;
			return tmp;
		} else{
			lruq->head->prev = NULL;
			return tmp;
		}
	}
	while (tmp && strcmp(tmp->filename, fn) != 0){
		tmp = tmp->next;
	}
	if (strcmp(tmp->filename, lruq->tail->filename)==0){
		lruq->tail = lruq->tail->prev;
		lruq->tail->next = NULL;
		return tmp;
	}
	tmp->next->prev = tmp->prev;
	tmp->prev->next = tmp->next;
	return tmp;
}

void move_to_tail(struct LRU_queue* lruq, char* fn) {
	struct LRU_node* deleted = deque(lruq, fn);
	enqueue(deleted, lruq);
}

struct cache* init_cache(int ht_size) {
	struct cache* c = (struct cache*) malloc(sizeof(struct cache));
	c->size = ht_size;
	c->total_file_size = 0;
	c->table = (struct Item**) malloc(c->size*sizeof(struct Item*));
	for (int i = 0; i < c->size; i++){
		c->table[i] = NULL;
	}
	c->lru_list = init_lruqueue();
	return c;
}

void deleteItem(struct cache* cache, char *fn){
	int index = hash_function(fn, cache->size);
	struct Item* curr = cache->table[index];
	if (strcmp(curr->filedata->file_name, fn) == 0){
		cache->table[index] = cache->table[index]->next;
		return;
	}
	while (curr->next != NULL && strcmp(curr->next->filedata->file_name, fn) != 0)
	{
		curr = curr->next;
	}
	curr->next = curr->next->next;
}

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	struct cache* sv_cache;
	/* add any other parameters you need */

	int *request_queue;
	int in;
	int out;

	pthread_t *threads;
};

pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data* data;
	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

struct Item* cache_lookup(struct cache* lrucache, char* fn) {
	int index = hash_function(fn, lrucache->size);
	struct Item* curr = lrucache->table[index];
	while (curr != NULL) {
		if (strcmp(curr->filedata->file_name, fn) == 0){
			return curr;
		}
		curr = curr->next;
	}
	return NULL;
}

int cache_evict(struct server* sv, int size){
	struct LRU_node* tmp = sv->sv_cache->lru_list->head;
	int evict_size = 0;
	struct Item* candidate = NULL;
	while (tmp != NULL && evict_size<size){
		candidate = cache_lookup(sv->sv_cache, tmp->filename);
		if (candidate->inuse == 0){
			evict_size += candidate->filedata->file_size;
		}
		else{
			return 0;
		}
		tmp = tmp->next;
	}
	if (evict_size < size){
		return 0;
	}
	else{
		tmp = sv->sv_cache->lru_list->head;
		evict_size = 0;
		while (tmp != NULL && evict_size<size){
			candidate = cache_lookup(sv->sv_cache, tmp->filename);
			if (candidate->inuse == 0){
				deleteItem(sv->sv_cache, candidate->filedata->file_name);
				sv->sv_cache->total_file_size -= candidate->filedata->file_size;
				tmp = deque(sv->sv_cache->lru_list, tmp->filename);	  
				evict_size += candidate->filedata->file_size;
			}
			tmp = tmp->next;
		}
	}
	return 1;
}

struct Item* cache_insert(struct server* sv, struct file_data* data) {
	if (data->file_size > sv->max_cache_size){
		return NULL;
	} 

	if (data->file_size > sv->max_cache_size - sv->sv_cache->total_file_size){
		if (cache_evict(sv, data->file_size + sv->sv_cache->total_file_size - sv->max_cache_size) == 0){
			return NULL;
		}
	}

	struct Item* node = createItem(data);
	int index = hash_function(data->file_name, sv->sv_cache->size);
	node->next = sv->sv_cache->table[index];
	sv->sv_cache->table[index] = node;
	sv->sv_cache->total_file_size += data->file_size;
	struct LRU_node* lru_node = init_lrunode(data->file_name);
	enqueue(lru_node, sv->sv_cache->lru_list);
	return node;
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	if (sv->max_cache_size==0){
		ret = request_readfile(rq);
		if (ret == 0) { /* couldn't read file */
			goto out;
		}
		request_sendfile(rq);
	} 
	else {
		pthread_mutex_lock(&cache_lock);
		struct Item* lookup_entry = cache_lookup(sv->sv_cache, data->file_name);

		if (lookup_entry != NULL){
			lookup_entry->inuse += 1;
			data->file_buf = strdup(lookup_entry->filedata->file_buf);
			data->file_size = lookup_entry->filedata->file_size;
			request_set_data(rq,data);
		} 
		else{
			/* read file, 
			* fills data->file_buf with the file contents,
			* data->file_size with file size. */
			pthread_mutex_unlock(&cache_lock);
			ret = request_readfile(rq);
			if (ret == 0) { /* couldn't read file */
				goto out;
			}
			pthread_mutex_lock(&cache_lock);
			lookup_entry = cache_insert(sv, data); // when inserting in cache_insert, dont forget inuse+=1 
			if (lookup_entry != NULL){
				lookup_entry->inuse += 1;
				move_to_tail(sv->sv_cache->lru_list, data->file_name);
			}
		}
		pthread_mutex_unlock(&cache_lock);
		request_sendfile(rq);
		pthread_mutex_lock(&cache_lock);
		if (lookup_entry != NULL){
			lookup_entry->inuse -= 1;
		}
		pthread_mutex_unlock(&cache_lock);
	}
out:
	request_destroy(rq);
}

void ReceiveRequest(struct server *sv)
{
	while (sv->exiting != 1){
		pthread_mutex_lock(&buffer_lock);
		while (sv->in==sv->out) {
			if (sv->exiting == 1){
				pthread_mutex_unlock(&buffer_lock);
				pthread_exit(NULL);
			}
			pthread_cond_wait(&empty, &buffer_lock);
		}
		int connfd = sv->request_queue[sv->out];
		sv->out = (sv->out+1)%(sv->max_requests+1);
		pthread_cond_signal(&full);
		pthread_mutex_unlock(&buffer_lock);
		do_server_request(sv, connfd);
	}
}

/* entry point functions */

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;
	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		/* Lab 4: create queue of max_request size when max_requests > 0 */
		if (max_requests > 0){
			sv->request_queue = (int*) Malloc((max_requests+1)*sizeof(int));
			sv->in = 0;
			sv->out = 0;
		}
		/* Lab 4: create worker threads when nr_threads > 0 */
		if (nr_threads > 0){
			sv->threads = (pthread_t*) Malloc(nr_threads*sizeof(pthread_t));
			for (int t = 0; t < nr_threads; t++) {
                pthread_create(&(sv->threads[t]), NULL, (void*)&ReceiveRequest, sv);
        	}
		}
		/* Lab 5: init server cache and limit its size to max_cache_size */
		if (max_cache_size > 0){
			sv->sv_cache = init_cache(SIZE);
		}
	}
	return sv;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd); 
	} else { 
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		pthread_mutex_lock(&buffer_lock);
        while ((sv->in-sv->out+sv->max_requests+1)%(sv->max_requests+1) == sv->max_requests) {
            pthread_cond_wait(&full, &buffer_lock);
        }
		sv->request_queue[sv->in] = connfd;
		sv->in = (sv->in + 1) % (sv->max_requests+1);
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&buffer_lock);
	}
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */

	sv->exiting = 1;

	pthread_cond_broadcast(&full);
	pthread_cond_broadcast(&empty);

	for (int t = 0; t < sv->nr_threads; t++) {
        pthread_join(sv->threads[t], NULL);
    }

	/* make sure to free any allocated resources */
	free(sv->request_queue);
	free(sv->threads);
	free(sv);
}