#ifndef _MYTHREAD_H
#define _MYTHREAD_H

#include <stdio.h>
#include <ucontext.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#define DO_NOTHING -1
#define CURR_THREAD_EXIT 0
#define RUN_NEXT_THREAD 1
#define JOIN 2
#define LOCK 3
#define UNLOCK 4

#define MAX_NODE_NUM 1000
#define MAX_MUTEX_NUM 1000

#define MAX_STACK_SAPCE 16384
#define MAX_THREAD_STACK_SPACE 4096

#define TIME_INTERVAL 50000 //50ms

typedef struct TCB{
	pthread_t thread_id;
	ucontext_t thread_context;
}TCB;

typedef struct Node{
	TCB * thread_block;
	int startTime;
	struct Node * next;
	struct Node * pre;
	struct Node * waitingList; 
}Node;

typedef struct Scheduler{
	struct Node *head; // the head of running queue 
	struct Node *tail; // the tail of running queue
	int threadSize; // the number of thread in the running queue
	int action; // next thing the scheduler will do
	int threadCreated;
	pthread_t joinId; // the id of thread you want to waiting for

	int curMutexID; // current lock id
	int mutexSize; //determine which MUTEX ID is free

	ucontext_t sched_context;
}Scheduler;

typedef struct maintenanceNode
{
	pthread_t thread_id;
	struct maintenanceNode *next;
	int startTime;
	int waitingTime;
}maintenance;

typedef struct mutex_t
{
	int isLock;
	int mutexID;

}my_pthread_mutex_t;

struct sigaction handler;
struct itimerval timer, otime;
char scheduler_stack[MAX_STACK_SAPCE];
char thread_stack[MAX_NODE_NUM][MAX_THREAD_STACK_SPACE];


int returnValue[MAX_NODE_NUM]; //return value of thread

Node *mutexWaitingList[MAX_MUTEX_NUM];



int my_pthread_create(pthread_t * thread, pthread_attr_t * attr, void *(*function)(void *), void * arg);

void my_pthread_yield();

void pthread_exit(void *value_ptr);

int my_pthread_join(pthread_t thread, void **value_ptr);

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t * mutexattr);

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

void curExit();

void runNextThread();

void joinThread();

void lock();

void unlock();

pthread_t addNewThread(TCB *block);



#endif