#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>


typedef struct queue_t {
    QueueNode head;
    QueueNode tail;
    int size;
    int max_size;
}queue_t;

//********************************************** ThreadInfo functions ************************************************//

ThreadInfo createThreadNode(int threadId) {
    ThreadInfo t = malloc(sizeof(*t));
    if (!t) return NULL;
    t->id = threadId;
    t->count = 0;
    t->dynamic_count = 0;
    t->static_count = 0;
    return t;
}

void destroyThreadNode(ThreadInfo t_info) {
    if (t_info)
        free(t_info);
}

ThreadInfo copyThreadNode(ThreadInfo t_info) {
    if (!t_info) return NULL;
    ThreadInfo copy = createThreadNode(t_info->id);
    copy->count = t_info->count;
    copy->static_count = t_info->static_count;
    copy->dynamic_count = t_info->dynamic_count;
    return copy;
}

//********************************************** QueueNode functions *************************************************//

QueueNode createQueueNode(int data , timeval arrival_time) {
    QueueNode newRequest = malloc(sizeof(*newRequest));
    if (!newRequest)
    {
        return NULL;
    }
    newRequest->data = data;
    newRequest->arrival_time = arrival_time;
    newRequest->dispatch_time = malloc(sizeof(*newRequest->dispatch_time));
    newRequest->thread_info = NULL;

    return newRequest;
}


void destroyQueueNode(QueueNode q_node) {
    destroyThreadNode(q_node->thread_info);
    free(q_node);
}


QueueNode queueNodeCopy(QueueNode q_node) {
    QueueNode copy = createQueueNode(q_node->data, q_node->arrival_time);
    copy->dispatch_time = q_node->dispatch_time;
    q_node->thread_info = copyThreadNode(q_node->thread_info);
    return copy;
}

//************************************************ Queue functions ***************************************************//


Queue createQueue(int max_size) {
    Queue queue = malloc(sizeof(*queue));
    if (!queue) return NULL;
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = max_size;
    return queue;
}



void destroyQueue(Queue queue) {
    if (!queue) return;
    QueueNode ptr = queue->head;
    QueueNode to_delete = queue->head;

    while (ptr) {
        ptr = ptr->next;
        destroyQueueNode(to_delete);
        to_delete = ptr;
    }

    free(queue);
}


bool queuePush(Queue queue, QueueNode to_add) {
    if (!queue || !to_add) return false;
    if (queue->size == queue->max_size) return false;

    QueueNode copy = queueNodeCopy(to_add);
    if (queueIsEmpty(queue)) {
        queue->head = copy;
        queue->tail = copy;
    }
    else {
        queue->tail->next = copy;
        copy->next = queue->tail;
        queue->tail = copy;
    }

    queue->size++;
    return true;
}


void queuePop(Queue queue) {
    if (!queue || queueIsEmpty(queue)) return;

    QueueNode to_pop = queue->head;
    queue->head = queue->head->next;
    if (queue->size == 1) queue->tail = NULL;
    queue->size--;
    destroyQueueNode(to_pop);
}



QueueNode queueFront(Queue queue) {
    return queueNodeCopy(queue->head);
}



void queuePopByData(Queue queue, int data){
    if (queueIsEmpty(queue)) return;

    QueueNode prev = queue->head;
    QueueNode ptr = prev->next;

    while (ptr) {
        if (ptr->data == data) {
            prev->next = prev->next->next;
            free(ptr);
            queue->size--;
            break;
        }
        prev = ptr;
        ptr = ptr->next;
    }
}



Queue queuePopRandom(Queue q, Queue to_remove) {
    if (queueIsEmpty(q)) return q;

    int to_delete_num = (q->size+1) / 2;
    int* delete_map = malloc(sizeof(*delete_map) * q->size);
    if (!delete_map) exit(1);

    for (int i = 0; i < q->size; ++i) {
        delete_map[i] = 0;
    }

    time_t t;
    int rand_i;
    srand(time(&t));

    while (to_delete_num != 0) {
        rand_i = rand() % q->size;
        if (delete_map[rand_i] == 1) {
            continue;
        }
        delete_map[rand_i] = 1;
        to_delete_num--;
    }

    Queue remaining_queue = createQueue(q->max_size);
    QueueNode ptr = q->head;

    for (int i = 0; i < q->size; i++) {
        if (delete_map[i] == 1)
            queuePush(to_remove, ptr);
        else
            queuePush(remaining_queue, ptr);
        ptr = ptr->next;
    }

    free(delete_map);
    return remaining_queue;
}


int queueSize(Queue queue){
    return queue->size;
}

int queueMaxSize(Queue queue){
    return queue->max_size;
}

bool queueIsEmpty(Queue queue){
    return queue->size == 0;
}

void queueInc(Queue queue) {
    queue->max_size++;
}


//******************************************** End of Queue functions ************************************************//