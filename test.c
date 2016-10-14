#include "my_pthread.h"

void * first_message() {
	printf("\tFirst\n");
	my_pthread_yield();
	printf("\tThird\n");
	int val = 1;
	pthread_exit(&val);
}

void * second_message() {
	printf("\tSecond\n");
	my_pthread_yield();
	printf("\tFourth\n");
	int val = 2;
	pthread_exit(&val);
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
	printf("\tThread 1 val: %d\n",*val1);
	int* val2;
	my_pthread_join(t2,(void**)&(val2));
	printf("\tThread 2 val: %d\n",*val2);
	printf("Above, you should have seen Starting followed by First, Second, Third, and Fourth printed out in order.\n");
	printf("The expected values are 1 and 2, the same as their thread ids.\n");


	return 0;
}