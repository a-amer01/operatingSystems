#ifndef HW3_QUEUE_H
#define HW3_QUEUE_H
#include <sys/time.h>
#include <stdbool.h>

typedef struct queue_t* Queue;

typedef struct timeval* timeval;


typedef struct thread_info {
    int id;
    int count;
    int static_count;
    int dynamic_count;

}* ThreadInfo;

typedef struct queue_node {
    int data;
    timeval arrival_time;
    timeval dispatch_time;
    ThreadInfo thread_info;
    struct queue_node *next;
} *QueueNode;


ThreadInfo createThreadNode(int threadId);
void destroyThreadNode(ThreadInfo t_info);
ThreadInfo copyThreadNode(ThreadInfo t_info);

QueueNode createQueueNode(int data , timeval timeval);
void destroyQueueNode(QueueNode q_node);
QueueNode queueNodeCopy(QueueNode q_node);

Queue createQueue(int max_size);
void destroyQueue(Queue queue);
bool queuePush(Queue queue, QueueNode node);
void queuePop(Queue queue);
void queuePopByData(Queue queue, int data);
Queue queuePopRandom(Queue queue, Queue to_remove);
QueueNode queueFront(Queue queue);
int queueSize(Queue queue);
int queueMaxSize(Queue queue);
bool queueIsEmpty(Queue queue);
Queue removeHalfElementsRandomly(Queue q, Queue deletedNodes);
void queueInc(Queue queue);
#endif //HW3_QUEUE_H