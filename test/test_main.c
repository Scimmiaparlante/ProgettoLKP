#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdlib.h>

#define NUM_THREADS		3
//#define NUM_SLOTS 		5

#define gettid() syscall(SYS_gettid)


//void (*thread_bodies[NUM_SLOTS])(void) = { };



void* thread_body_common(void* sched_slot)
{
	int fd, i;	
	char sched_mess_buf[20];
	char buf[100];
		
	//communicate my data to the scehduler
	sprintf(sched_mess_buf, "slice:%d,%ld", (int)(long long int)sched_slot, gettid());

	fd = open("/dev/scheduler_setting", O_RDWR);
	write(fd, sched_mess_buf, strlen(sched_mess_buf));
	read(fd, buf, 100);
	printf("%s\n", buf);
	close(fd);


	//block on parking
	fd = open("/dev/scheduler_parking", O_RDWR);
	read(fd, NULL, 0);
	close(fd);

	//do things ...
//	thread_bodies[ (int)(long long int)sched_slot ]();
	while(1) {
		int cont;

		for(i = 0; i < 1000000000; i++)
			cont += rand();

		printf("%d -- result:%d", (int)(long long int)sched_slot, cont);
	}

	pthread_exit(NULL);

	return NULL;
}




int main()
{
	char buf[100];
	int ret, i;
	pthread_t mythread[NUM_THREADS];

	int fd = open("/dev/scheduler_setting", O_RDWR);

	write(fd, "num_slices:5;slice_size:100", 28);
	read(fd, buf, 100);

	close(fd);

	printf("%s\n", buf);

	//create the threads (as an argument I communicate the scehduling slot, since only the thread itself knows its tid)
	ret = pthread_create(mythread[1], NULL , thread_body_common, (void*)2);
	ret |= pthread_create(mythread[2], NULL , thread_body_common, (void*)1);
	if (ret) {
		printf("Errore nella creazione dei thread\n");
	}

	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(mythread[i], NULL);

	return 0;
}