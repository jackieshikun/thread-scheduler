#include "my_pthread.h"

Scheduler *scheduler;

int isSchdulerCreated = 0;
void ARM_handler(){
	printf("Interval\n");
	my_pthread_yield();
}

long int get_time_stamp(){
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	return 1000000*current_time.tv_sec + current_time.tv_usec;
}

void maintenance_queue_init(){
	mainQueue = (maintenanceQueue *)malloc(sizeof(maintenanceQueue));
	mainQueue->head = NULL;
	mainQueue->tail = NULL;
}
void add_thread2mt_queue(pthread_t thread_id, long int startTime){
	maintenanceNode *newNode = (maintenanceNode*)malloc(sizeof(maintenanceNode));
	newNode->next = NULL;
	newNode->thread_id = thread_id;
	newNode->startTime = startTime;
	newNode->runningTime = 0;
	newNode->endTime = 0;
	if(mainQueue->head == NULL){
		mainQueue->head = mainQueue->tail = newNode;
	}else{
		mainQueue->tail->next = newNode;
		mainQueue->tail = newNode;
	}
}
maintenanceNode* find_target_mt_node(pthread_t thread_id){
	maintenanceNode * cur = mainQueue->head;
	while(cur != NULL){
		if(cur->thread_id == thread_id)
			return cur;
		cur = cur->next;
	}
	return cur;
}
void set_thread_end_time(pthread_t thread_id, long int endTime){
	maintenanceNode *target = find_target_mt_node(thread_id);
	if(target != NULL){
		target->endTime = endTime;
	}
}
void set_thread_running_time(pthread_t thread_id,long int curTime){
	maintenanceNode *target = find_target_mt_node(thread_id);
	if(target != NULL){
		target->runningTime += curTime - target->startTime;
	}
}



void timer_init(int interval){
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = interval;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = interval;
	setitimer(ITIMER_REAL, &timer, &otime);
}
void timer_cancel(){
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &timer, &otime);
}
void schedule(void){
	//printf("entering sizeschedule,action = %d\n",scheduler->action);
	while(scheduler->threadSize > 0){
		//do priority thing here
		switch(scheduler->action){
			case CURR_THREAD_EXIT: curExit(); break;
			case RUN_NEXT_THREAD: 
			timer_init(TIME_INTERVAL);
			runNextThread(); 
			timer_cancel();
			break;
			case JOIN: joinThread(); break;
			case LOCK: lock(); break;
			case UNLOCK: unlock(); break;
			default: printf("schedule default\n"); return;
		}

	}

}
void lock(){
	printf("Entering lock function\n");
	Node * mutexWaitingListTail = mutexWaitingList[scheduler->curMutexID];
	while(mutexWaitingListTail->next != NULL) mutexWaitingListTail = mutexWaitingListTail->next;

	mutexWaitingListTail = scheduler->head;
	scheduler->head = scheduler->head->next;
	mutexWaitingListTail->next = NULL;

	if(scheduler->head == NULL){
		printf("deadlock!\n");
		exit(0);
	}
	scheduler->head->pre = NULL;
	scheduler->action = CURR_THREAD_EXIT;
	swapcontext(&scheduler->sched_context,&scheduler->head->thread_block->thread_context);

}
void unlock(){
	printf("Entering unlock function");
	Node * mutexWaitingListHead = mutexWaitingList[scheduler->curMutexID];
	if(mutexWaitingListHead != NULL){
		scheduler->tail->next = mutexWaitingListHead;
		mutexWaitingListHead->pre = scheduler->tail;
		scheduler->tail = mutexWaitingListHead;
		mutexWaitingList[scheduler->curMutexID] = mutexWaitingListHead->next;

		scheduler->tail->next = NULL;
	}
	scheduler->action = CURR_THREAD_EXIT;
	swapcontext(&scheduler->sched_context,&scheduler->head->thread_block->thread_context);


}

pthread_t addNewThread(TCB *block){
	//printf("entering addNewThread\n");
	if(scheduler->threadSize < MAX_NODE_NUM){
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
		scheduler->threadSize++ ;
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

	scheduler->threadSize --;
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
Node * getTargetInMutexWaitingList(pthread_t join_id){
	Node * search = NULL;
	int i;
	for(i=0;i<scheduler->mutexSize;i++){
		search = mutexWaitingList[i];
		while(search != NULL){
			if(search->thread_block->thread_id == join_id)
				return search;
			search = search->next;
		}

	}
	return NULL;
}
void joinThread(){
	//printf("Entering joinThread\n");
	//printf("schedule:%x\n",&scheduler->sched_context);
	Node * target = getTargetJoinThreadPosition(scheduler->head,scheduler->joinId);
	if(target == NULL)
		target = getTargetInMutexWaitingList(scheduler->joinId);
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
			swapcontext(&scheduler->sched_context,&scheduler->head->thread_block->thread_context);
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

void scheduler_init(){
	//printf("Entering scheduler_init\n");
	scheduler = (Scheduler *)malloc(sizeof(Scheduler));
	scheduler->threadSize = 0;
	scheduler->head = NULL;
	scheduler->tail = NULL;
	scheduler->action = DO_NOTHING;
	scheduler->mutexSize = 0;


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
	if(scheduler->threadSize >= MAX_NODE_NUM)
		return -1;
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
	returnValue[scheduler->head->thread_block->thread_id] = *((int*) value_ptr);
	swapcontext(&(scheduler->head->thread_block->thread_context),&(scheduler->sched_context));

}

int my_pthread_join(pthread_t thread, void **value_ptr){
	//printf("Entering join\n");
	scheduler->action = JOIN;
	scheduler->joinId = thread;
	printQueue("my_pthread_join");
	if(value_ptr != NULL)
		*value_ptr = &returnValue[scheduler->joinId];
	swapcontext(&(scheduler->head->thread_block->thread_context),&(scheduler->sched_context));
	return 0;
}

int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t * mutexattr){
	//printf("my_pthread_mutex_init");
	if(isSchdulerCreated){
		scheduler_init();
		isSchdulerCreated = 1;
		handler.sa_handler = ARM_handler;
		sigaction(SIGALRM,&handler, NULL);		
	}
	if(scheduler->mutexSize > MAX_MUTEX_NUM)
		return -1;
	mutex = (my_pthread_mutex_t *)malloc(sizeof(my_pthread_mutex_t));
	mutex->isLock = 0;
	mutex->mutexID = scheduler->mutexSize++;
	return 0;

}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){
	//printf("my_pthread_mutex_lock");
	if(mutex == NULL)
		return -1;
	if(mutex->isLock == 1){
		scheduler->curMutexID = mutex->mutexID;
		scheduler->action = LOCK;
		swapcontext(&scheduler->head->thread_block->thread_context,&scheduler->sched_context);
	}else
		mutex->isLock = 0;
	return 0;

}
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){
	//printf("my_pthread_mutex_unlock\n");
	if(mutex == NULL || mutex->isLock == 0)
		return -1;
	scheduler->action = UNLOCK;
	scheduler->curMutexID = mutex->mutexID;
	swapcontext(&scheduler->head->thread_block->thread_context,&scheduler->sched_context);
	mutex->isLock = 0;
	return 0;
}
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){
	if(mutex == NULL)
		return -1;
	free(mutex);
	scheduler->mutexSize--;
	return 0;
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
