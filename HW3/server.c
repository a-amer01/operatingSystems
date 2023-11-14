#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "queue.h"
#define MAX_SCHED_NAME 7

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requethreads_info sent to this port number.
// Most of the work is done within routines written in request.c
//
void acceptRequest(int queue_size, int connfd, char* sched_name, QueueNode request,int max_queue_size);
void *handleThreadRequest(void *arg);
int getPolicyCode(char* policy_str);
void scheduleNextRequest(int queue_size, int connfd, char* sched_name, QueueNode request);

Queue request_queue;
pthread_mutex_t queue_lock;
pthread_cond_t normal_cond;
pthread_cond_t block_cond;
int working_threads_num;


typedef struct policy_map {
    char* policy;
    int code;
} PolicyMap;

PolicyMap pm[7] = {
        {"block", 1},
        {"dh", 2},
        {"dt", 3},
        {"random", 4},
        {"bf", 5},
        {"dynamic", 6},
        {0,0}
};


// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[], int *total_threads, char *sched_name, int *queue_size, int *max_queue_size)
{
    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <port> <threads> <queue_size> <schedalg> <max_size(optional)>\n", argv[0]);
        exit(1);
    }
    *port = atoi(argv[1]);
    *total_threads = atoi(argv[2]);
    *queue_size = atoi(argv[3]);
    strcpy(sched_name, argv[4]);

       if(strcmp(sched_name,"dynamic")==0){
            if(argc != 6){
                fprintf(stderr, "Usage: %s with dynamic <port> <threads> <queue_size> <schedalg> <max_size>\n", argv[0]);
                exit(1);
            }else{
                *max_queue_size = atoi(argv[5]);
            }
       }
    

}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    int total_thread_num, queue_size, max_queue_size = 0;
    struct sockaddr_in clientaddr;
    char sched_name[MAX_SCHED_NAME]; // 7 because random is the biggest string;
    QueueNode req;

    getargs(&port, argc, argv, &total_thread_num, sched_name, &queue_size,&max_queue_size);

    // init the locks and queues
    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&normal_cond, NULL);
    pthread_cond_init(&block_cond, NULL);
    working_threads_num = 0;
    request_queue = createQueue(queue_size);

    // init threads and their statistics
    pthread_t *workers = malloc(sizeof(pthread_t) * total_thread_num);
    ThreadInfo *threads_info = malloc(sizeof(ThreadInfo) * total_thread_num);
    int i = 0;
    while (i < total_thread_num)
    {
        threads_info[i] = createThreadNode(i);
        pthread_create(&workers[i], NULL, handleThreadRequest, threads_info[i]);
        ++i;
    }

    // start the work
    listenfd = Open_listenfd(port);
    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        timeval arrive_time = malloc(sizeof(*arrive_time));
        gettimeofday(arrive_time, NULL);

        req = createQueueNode(connfd, arrive_time);

        pthread_mutex_lock(&queue_lock);

        acceptRequest(queue_size, connfd, sched_name, req,max_queue_size);

        pthread_cond_signal(&normal_cond);
        pthread_mutex_unlock(&queue_lock);
    }
}


void *handleThreadRequest(void *arg)
{
    ThreadInfo st = (ThreadInfo) arg;
    timeval received_time = malloc(sizeof(struct timeval)); // maybe put this in while
    QueueNode request;

    while(1) {
        pthread_mutex_lock(&queue_lock);
        for (; queueIsEmpty(request_queue);) {
            pthread_cond_wait(&normal_cond, &queue_lock);
        }

        // get the time of day.
        gettimeofday(received_time, NULL);
        // get the request
        request = queueFront(request_queue);
        queuePop(request_queue);

        (st->count)++;
        timeval d_time = malloc(sizeof(*d_time));
        timersub (received_time , request->arrival_time, d_time);
        request->dispatch_time->tv_sec = d_time->tv_sec;
        request->dispatch_time->tv_usec = d_time->tv_usec;
        free(d_time);

        request->thread_info = st;

        // count workers
        ++working_threads_num;

        pthread_mutex_unlock(&queue_lock);

        if (request != NULL) {
            requestHandle(request);
            Close(request->data );
            free(request);
        }

        pthread_mutex_lock(&queue_lock);
        --working_threads_num;
        pthread_cond_signal(&block_cond);
        pthread_mutex_unlock(&queue_lock);
    }
}



inline void acceptRequest(int queue_size, int connfd, char* sched_name, QueueNode request ,int max_queue_size) {
    QueueNode tmp_request;
    int policy_code = getPolicyCode(sched_name);

    if (working_threads_num + queueSize(request_queue) < queueMaxSize(request_queue)) {
        queuePush(request_queue,request);
        return;
    }

    switch (policy_code) {
        case 1: {
            while (queueSize(request_queue) + working_threads_num >= queueMaxSize(request_queue)) {
                pthread_cond_wait(&block_cond, &queue_lock);
            }
            queuePush(request_queue, request);
            return;
        }
        case 2: {
            if (queueIsEmpty(request_queue)) {
                destroyQueueNode(request);
                Close(connfd);
                return;
            }

            // handle oldest request
            tmp_request = queueFront(request_queue);
            Close(tmp_request->data);
            queuePop(request_queue);
            destroyQueueNode(tmp_request);

            // insert new request
            queuePush(request_queue, request);
            return;
        }
        case 3: {
            destroyQueueNode(request);
            Close(connfd);
            return;
        }
        case 4: {
            if (queueSize(request_queue)) {
                destroyQueueNode(request);
                Close(connfd);
                return;
            }

            Queue deleted_vals, old_queue;
            deleted_vals = createQueue(queue_size);
            old_queue = request_queue;
            request_queue = queuePopRandom(request_queue, deleted_vals);
            destroyQueue(old_queue);
            queuePush(request_queue, request);

            for (;!queueIsEmpty(deleted_vals);) {
                tmp_request = queueFront(deleted_vals);
                Close(tmp_request->data);
                queuePop(deleted_vals);
                destroyQueueNode(tmp_request);
            }

            destroyQueue(deleted_vals);
            return;
            return;
        }
        case 5: {
            if (queueSize(request_queue) + working_threads_num >= queue_size) {
                while (queueSize(request_queue) > 0) {
                    pthread_cond_wait(&block_cond, &queue_lock);
                }
            }
            destroyQueueNode(request);
            Close(connfd);
            //queuePush(request_queue, request);
            return;
        }
        case 6: {
            if(queueSize(request_queue) <= max_queue_size){
                queueInc(request_queue);
            }

            destroyQueueNode(request);
            Close(connfd);
            return;
        }
        default: {
            break;
        }
    }
}



int getPolicyCode(char* policy_str) {
    char *name;
    for (int i = 0; i < 6; i++) {
        name = pm[i].policy;
        if (!strcmp(name, policy_str))
            return pm[i].code;
    }
    return 0;
}