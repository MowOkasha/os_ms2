#include "main.h"
#include <stdlib.h>

void init_queue(Queue* queue) {
    queue->front = queue->rear = NULL;
}

void enqueue(Queue* queue, Process* process) {
    QueueNode* node = malloc(sizeof(QueueNode));
    node->process   = process;
    node->next      = NULL;
    if (queue->rear) {
        queue->rear->next = node;
        queue->rear       = node;
    } else {
        queue->front = queue->rear = node;
    }
}

Process* dequeue(Queue* queue, int param) {
    (void)param; // unused for now
    if (!queue->front) return NULL;
    QueueNode* node = queue->front;
    Process*   p    = node->process;
    queue->front    = node->next;
    if (!queue->front) queue->rear = NULL;
    free(node);
    return p;
}