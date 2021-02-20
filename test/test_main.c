#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdlib.h>

#define NUM_THREADS		5
#define NUM_SLICES 		8
#define SLICE_SIZE		1500  //milliseconds

#define gettid() syscall(SYS_gettid)



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

	while(1) {

		//------------------- THREAD SPECIFIC OPS ------------------------------------------------------------		
		unsigned int cont;

		for(i = 0; i < 80000000/*0*/; i++)
			cont += i;

		printf("[%d, %ld] ...\n", (int)(long long int)sched_slot, gettid());//result:%u\n", (int)(long long int)sched_slot, gettid(), cont);
		//-----------------------------------------------------------------------------------------------------
	}

	close(fd);


	pthread_exit(NULL);

	return NULL;
}



int main()
{
	char buf[100];
	int ret, i;
	pthread_t mythread[NUM_THREADS];
	char mess_buff[50];

	int fd = open("/dev/scheduler_setting", O_RDWR);

	sprintf(mess_buff, "num_slices:%u;slice_size:%u", NUM_SLICES, SLICE_SIZE);

	write(fd, mess_buff, strlen(mess_buff)+1);
	read(fd, buf, 100);

	printf("%s\n", buf);

	//create the threads (as an argument I communicate the scehduling slot, since only the thread itself knows its tid)
	ret = pthread_create(&mythread[0], NULL , thread_body_common, (void*)2);
	ret |= pthread_create(&mythread[1], NULL , thread_body_common, (void*)1);
	ret |= pthread_create(&mythread[2], NULL , thread_body_common, (void*)4);
	ret |= pthread_create(&mythread[3], NULL , thread_body_common, (void*)5);
	ret |= pthread_create(&mythread[4], NULL , thread_body_common, (void*)6);

	if (ret) {
		printf("Errore nella creazione dei thread\n");
	}


	write(fd, "ctrl:start;", 12);

	sleep(200);

	write(fd, "ctrl:stop;", 11);

	close(fd);


	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(mythread[i], NULL);

	return 0;
}