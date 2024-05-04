// sched.c
#include "queue.h"
#include "sched.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
// static int slot[MAX_PRIO]; // static : Can be used for all files

#endif
void check_size_ready_queue(int k)
{
	printf("Size of ready_queue[%d] = %d\n", k, mlq_ready_queue[k].size);
}

int queue_empty(void)
{
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if (!empty(&mlq_ready_queue[prio]))
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void)
{
#ifdef MLQ_SCHED
	int i;

	for (i = 0; i < MAX_PRIO; i++)
		mlq_ready_queue[i].size = 0;
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
 
 
struct pcb_t *get_mlq_proc(void)
{
	
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 */
	struct pcb_t *proc = NULL;
	
	static int currentPriority = 0; // Start with queue have the highest priority
	static int currentSpace = MAX_PRIO; // Start with slot = MAX_PRIO
	
	pthread_mutex_lock(&queue_lock);
	
	// All of queue are empty, cannot get any process
	if(queue_empty() != -1) {
		pthread_mutex_unlock(&queue_lock);
		return NULL;
	}
	
	// Get the process with the greatest priority
	for(int i = 0; i < currentPriority; i++) {
		if(!empty(&mlq_ready_queue[i])) {
			currentPriority = i;
			currentSpace = MAX_PRIO - i - 1;
			proc = dequeue(&mlq_ready_queue[i]);
			pthread_mutex_unlock(&queue_lock);
			return proc;		
		}
	}
	
	// There is no process with priority better than current priority, check the queue with prio = currentPriority
	// That queue must still have slot
	if(!empty(&mlq_ready_queue[currentPriority]) && currentSpace > 0) {
		proc = dequeue(&mlq_ready_queue[currentPriority]);
		currentSpace = currentSpace - 1;
		pthread_mutex_unlock(&queue_lock);
		return proc;
	}
	
	// Must take the greatest priority > currentPriority (if any)
	for(int i = currentPriority + 1; i < MAX_PRIO; i++) {
		if(!empty(&mlq_ready_queue[i])) {
			currentPriority = i;
			currentSpace = MAX_PRIO - i - 1;
			proc = dequeue(&mlq_ready_queue[i]);
			pthread_mutex_unlock(&queue_lock);
			return proc;		
		}
	}
	
	// Must use that queue with currentPriority
	if(!empty(&mlq_ready_queue[currentPriority])) {
		proc = dequeue(&mlq_ready_queue[currentPriority]);
	}
	
	
	pthread_mutex_unlock(&queue_lock);
	return proc;


}

void put_mlq_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

struct pcb_t *get_proc(void)
{
	return get_mlq_proc();
}

void put_proc(struct pcb_t *proc)
{
	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t *proc)
{
	return add_mlq_proc(proc);
}
#else
struct pcb_t *get_proc(void)
{
	struct pcb_t *proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	pthread_mutex_lock(&queue_lock);
	if (empty(&ready_queue))
	{
		while (!empty(&run_queue))
		{
			enqueue(&ready_queue, dequeue(&run_queue));
		}
	}
	proc = dequeue(&ready_queue);
	pthread_mutex_unlock(&queue_lock);

	return proc;
}

void put_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}
#endif
