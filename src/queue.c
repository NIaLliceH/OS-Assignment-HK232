#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Check if queue is empty or not
int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */

        if(q == NULL) return; // q is not initialized
        if(q->size == MAX_QUEUE_SIZE) return; // queue [q] is full, cannot put more process
        
        // [q] has empty slot and ready to add new process
        q->proc[q->size] = proc;
        q->size = q->size + 1;
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */

        if(q == NULL || q->size == 0) return NULL; // q is not initialized or q is empty, cannot remove process
        
        // Find the index which has the lowest value -> the priority is highest
        int index = 0;
        for(int i = 1; i < q->size; i++) {
                if(q->proc[i]->priority < q->proc[index]->priority) {
                        index = i;
                }
        }
        struct pcb_t* remove_process = q->proc[index]; // Get the process with highest priority

        // Remove that process, push all processes after that up 1 position 
        for(int i = index; i < q->size - 1; i++) {
                q->proc[i] = q->proc[i + 1];
        }
        q->size = q->size - 1;
        q->proc[q->size] = NULL;
        return remove_process;
}
