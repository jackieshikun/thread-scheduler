#ifndef _MYTHREAD_H
#define _MYTHREAD_H

#include <stdio.h>
#include <ucontext.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#define DO_NOTHING -1
#define CURR_THREAD_EXIT 0
#define RUN_NEXT_THREAD 1
#define JOIN 2

#define MAX_NODE_NUM 1000
#define MAX_STACK_SAPCE 16384
#define MAX_THREAD_STACK_SPACE 8192
typedef struct TCB{
	pthread_t thread_id;
	ucontext_t thread_context;
}TCB;

typedef struct Node{
	TCB * thread_block;
	struct Node * next;
	struct Node * pre;
	struct Node * waitingList; 
}Node;

typedef struct Scheduler{
	struct Node *head;
	struct Node *tail;
	int maxSize;
	int size;
	int action;
	int threadCreated;
	pthread_t joinId;

	ucontext_t sched_context;
}Scheduler;
struct sigaction handler;
char scheduler_stack[MAX_STACK_SAPCE];
char thread_stack[MAX_NODE_NUM][MAX_THREAD_STACK_SPACE];

typedef pthread_mutex_t my_pthread_mutex_t;


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

int joinThread();

pthread_t addNewThread(TCB *block);



#endif