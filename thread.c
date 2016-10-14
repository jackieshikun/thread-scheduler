#include "my_pthread.h"

Scheduler *scheduler;
int isSchdulerCreated = 0;

void ARM_handler(){
	my_pthread_yield();
}

void schedule(void){
	//printf("entering schedule,action = %d\n",scheduler->action);
	while(scheduler->size > 0){
		switch(scheduler->action){
			case CURR_THREAD_EXIT: curExit(); break;
			case RUN_NEXT_THREAD: runNextThread(); break;
			case JOIN: joinThread(); break;
			default: printf("schedule default\n"); return;
		}

	}

}
pthread_t addNewThread(TCB *block){
	//printf("entering addNewThread\n");
	if(scheduler->size < scheduler->maxSize){
		block->thread_id = scheduler->threadCreated++;
		Node* newNode = (Node *)malloc(sizeof(Node));
		if (newNode == NULL){
			printf("fail to allocate memory\n");
			exit(0);
		}
		newNode->thread_block = block;
		newNode->next = NULL;
		newNode->waitingList = NULL;
		if(scheduler->head == NULL){
			newNode->pre = NULL;
			scheduler->tail = scheduler->head = newNode;
		}else{
			scheduler->tail->next = newNode;
			newNode->pre = scheduler->tail;
			scheduler->tail = newNode;
		}
		scheduler->size++ ;
	}else{
		printf("Thread poll is full\n");
	}
	printQueue("addNewThread");
	return block->thread_id;
}
void curExit(){
	//printf("Entering curexit\n");
	Node * deleteNode = scheduler->head;

	scheduler->head = scheduler->head->next;
	scheduler->head->pre = NULL;

	Node * curWaitingThread = deleteNode->waitingList;
	while(curWaitingThread != NULL){
		Node * nextThread = curWaitingThread->next;
		//nextThread->pre = NULL;

		scheduler->tail->next = curWaitingThread;
		curWaitingThread->pre = scheduler->tail;
		scheduler->tail = curWaitingThread;

		curWaitingThread = nextThread;
	}

	free(deleteNode);

	scheduler->size --;
	scheduler->action = CURR_THREAD_EXIT;
	if(scheduler->head != NULL)
		swapcontext(&(scheduler->sched_context),&(scheduler->head->thread_block->thread_context));
	printQueue("curExit");
}

void runNextThread(){
	//printf("Entering runNextThread\n");
	if(scheduler->tail != scheduler->head){
		scheduler->tail->next = scheduler->head;
		scheduler->tail->next->pre = scheduler->tail;

		scheduler->head = scheduler->head->next;
		scheduler->head->pre = NULL;

		scheduler->tail = scheduler->tail->next;
		scheduler->tail->next = NULL;
	}
	scheduler->action = CURR_THREAD_EXIT;
	printQueue("runNextThread");
	swapcontext(&(scheduler->sched_context), &(scheduler->head->thread_block->thread_context));
}

Node* getTargetJoinThreadPosition(Node *root, pthread_t join_id){
	//printf("Entering getTargetJoinThreadPosition\n");
	int isInWaitingList = 0;
	while(root != NULL){
		if(root->thread_block->thread_id == join_id)
			break;
		Node * join = root->waitingList;
		while(join != NULL){
			if(join->thread_block->thread_id == join_id){
				isInWaitingList = 1;
				break;
			}
			join = join->next;
		}
		if(isInWaitingList)
			break;
		root = root->next;
	}
	//printf("target = %d\n",root->thread_block->thread_id);
	printQueue("getTargetJoinThreadPosition");
	return root;
}

int joinThread(){
	int test;
	//printf("Entering joinThread\n");
	//printf("schedule:%x\n",&scheduler->sched_context);
	Node * target = getTargetJoinThreadPosition(scheduler->head,scheduler->joinId);
	if(target != NULL){
		if(target->waitingList == NULL){
			target->waitingList = scheduler->head;
			scheduler->head->pre = NULL;
			scheduler->head = scheduler->head->next;
			target->waitingList->next = NULL;

			if(scheduler->head == NULL){
				printf("head == NULL,exit\n");
				exit(0);
			}
			scheduler->action = CURR_THREAD_EXIT;
			printQueue("joinThread1");
			test = swapcontext(&scheduler->sched_context,&scheduler->head->thread_block->thread_context);
			//printf("test = %d Hello world\n",test);
		}else{
			Node * curWaitingThread = target->waitingList;
			while(curWaitingThread->next != NULL) curWaitingThread = curWaitingThread->next;
			
			curWaitingThread->next = scheduler->head;
			scheduler->head->pre = curWaitingThread;
			scheduler->head = scheduler->head->next;
			curWaitingThread->next->next = NULL;

			if(scheduler->head == NULL){
				printf("DeadLock\n");
				exit(0);
			}

			scheduler->action = CURR_THREAD_EXIT;
			printQueue("joinThread2");
			swapcontext(&(scheduler->sched_context),&(scheduler->head->thread_block->thread_context));

		}
	}else{
		scheduler->action = CURR_THREAD_EXIT;
		printQueue("joinThread3");
		swapcontext(&(scheduler->sched_context),&(scheduler->head->thread_block->thread_context));
	}
	//printf("WTF\n");
	return 0;
}

int scheduler_init(){
	//printf("Entering scheduler_init\n");
	scheduler = (Scheduler *)malloc(sizeof(Scheduler));
	scheduler->size = 0;
	scheduler->maxSize = MAX_NODE_NUM;
	scheduler->head = NULL;
	scheduler->tail = NULL;
	scheduler->action = DO_NOTHING;

	getcontext(&scheduler->sched_context);
	scheduler->sched_context.uc_link = 0;
	scheduler->sched_context.uc_stack.ss_sp = scheduler_stack;
	scheduler->sched_context.uc_stack.ss_size = sizeof(scheduler_stack);
	makecontext(&scheduler->sched_context,schedule,0);
	//printf("schedule address = %x\n",&schedule);

	TCB *main_block = (TCB *)malloc(sizeof(TCB));
	getcontext(&main_block->thread_context); 
	main_block->thread_context.uc_link = &scheduler->sched_context;
	printQueue("scheduler_init");
	addNewThread(main_block);
	//printf("head:%d,address %x\n", main_block->thread_id,&main_block->thread_context);
}

int my_pthread_create(pthread_t * thread, pthread_attr_t * attr, void *(*function)(void *), void * arg){
	//printf("Entering pthread_create\n");
	if(isSchdulerCreated == 0){
		
		scheduler_init();
		isSchdulerCreated = 1;
		handler.sa_handler = ARM_handler;
		sigaction(SIGALRM,&handler, NULL);
	}

	TCB * thread_block = (TCB *)malloc(sizeof(TCB));
	getcontext(&thread_block->thread_context);
	thread_block->thread_context.uc_link = &scheduler->sched_context;
	thread_block->thread_context.uc_stack.ss_sp = thread_stack[scheduler->threadCreated];
	thread_block->thread_context.uc_stack.ss_size = MAX_THREAD_STACK_SPACE;
	makecontext(&thread_block->thread_context,function,1,arg);
	//printf("function address = %x\n",function);
	*thread = addNewThread(thread_block);
	//printf("head:%d,address %x\n", *thread,&thread_block->thread_context);
	printQueue("my_pthread_create");
	return 0;

}

void my_pthread_yield(){
	//printf("Entering yield\n");
	scheduler->action = RUN_NEXT_THREAD;
	//printQueue("my_pthread_yield");
	//printf("jointhread address = %x\n",&joinThread);
	//printf("head:%d,address %x\n",scheduler->head->thread_block->thread_id, &(scheduler->head->thread_block->thread_context));
	//printf("schedule:%x\n",&scheduler->sched_context);
	swapcontext(&scheduler->head->thread_block->thread_context,&scheduler->sched_context);
	//printf("yield success\n");
}

void pthread_exit(void *value_ptr){
	//printf("Entering exit\n");
	scheduler->action = CURR_THREAD_EXIT;
	printQueue("pthread_exit");
	swapcontext(&(scheduler->head->thread_block->thread_context),&(scheduler->sched_context));

}

int my_pthread_join(pthread_t thread, void **value_ptr){
	//printf("Entering join\n");
	scheduler->action = JOIN;
	scheduler->joinId = thread;
	printQueue("my_pthread_join");
	swapcontext(&(scheduler->head->thread_block->thread_context),&(scheduler->sched_context));
	return 0;
}

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t * mutexattr){


}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){


}
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){

}
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){


}
void printQueue(char *name){
	Node *print = scheduler->head;
	//printf("%s  ",name);
	while(print != NULL){
		//printf("%d-> ",print->thread_block->thread_id);
		/*
		Node *join = print->waitingList;
		while(join != NULL){
			printf("%d-> ",join->thread_block->thread_id);
			join = join->next;
		}
		*/
		print = print->next;
	}
	//printf("\n");

}
