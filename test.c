#include "my_pthread.h"
my_pthread_mutex_t* pcm;
int shared = 0;
void * first_message() {
	printf("\tFirst\n");
	my_pthread_yield();
	printf("\tThird\n");
	char* val = "first_message";
	pthread_exit(&val);
}

void * second_message() {
	printf("\tSecond\n");
	my_pthread_yield();
	printf("\tFourth\n");
	char* val = "second_message";
	pthread_exit(&val);
}
void * producer() {
	int count = 0;
	do {
		printf("producing: %d\n",shared);
		count++;
		my_pthread_mutex_lock(&pcm);
		shared += 2;
		my_pthread_yield();
		printf("afteryield\n");
		my_pthread_mutex_unlock(&pcm);
	}while(count<10);
}

void * consumer() {
	int count = 0;
	do {
		printf("consuming: %d\n",shared);
		count++;
		my_pthread_mutex_lock(&pcm);
		shared--;
		my_pthread_yield();
		my_pthread_mutex_unlock(&pcm);
	}while(count<10);
}

void main(void) {

	pthread_t t1,t2,w1,r1,r2,r3,r4,pct1,pct2;

	printf("Threading Proof of Concept\n");
	printf("first_message address = %x\n",&first_message);
	printf("second_message address = %x\n",&second_message);
	my_pthread_create(&t1, NULL, &first_message, NULL);
	my_pthread_create(&t2, NULL, &second_message, NULL);
	printf("\tStarting...t1=%d,t2=%d\n",t1,t2);
	int* val1;
	my_pthread_join(t1,(void**)&(val1));
	printf("\tThread 1 val: %s\n",*val1);
	int* val2;
	my_pthread_join(t2,(void**)&(val2));
	printf("\tThread 2 val: %s\n",*val2);
	printf("Above, you should have seen Starting followed by First, Second, Third, and Fourth printed out in order.\n");
	printf("The expected values are 1 and 2, the same as their thread ids.\n");

	printf("\n\n\nProducer-Consumer Problem\n");

	my_pthread_mutex_init(&pcm,NULL);

	my_pthread_create(&pct1,NULL,&producer,NULL);
	my_pthread_create(&pct2,NULL,&consumer,NULL);

	my_pthread_join(pct1,NULL);
	my_pthread_join(pct2,NULL);

	printf("%d at the end. 10 expected.",shared);

	printf("\n\n\nThe Readers-Writers Problem\n");
	printf("This problem uses both a mutex and a conditional variable.\n");


	return 0;
}