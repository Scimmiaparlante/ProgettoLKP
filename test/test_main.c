#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>




int main()
{
	char buf[100];

	int fd = open("/dev/scheduler_setting", O_RDWR);

	write(fd, "num_slices:5;slice_size:100", 28);
	write(fd, "slice:2,40;slice:1,20", 22);

	read(fd, buf, 100);

	printf("%s\n", buf); 

	close(fd);
}