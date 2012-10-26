#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

int main()
{
	struct stat sb;
	int fd = open("file", O_RDWR | O_CREAT | O_APPEND, 0644);
	int newfd;
	if(fstat(fd,&sb));
	int *ptr;
	char *qw;
	ptr = mmap(NULL,sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd,(off_t)0);
	if (ptr == MAP_FAILED) {
		close(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}
	printf("%d\n",sb.st_size);

	char *buf = (char *)ptr;
	int i=0;
	//printf("%d\n",*buf - '0');
	while(i++<sb.st_size)
	{
		newfd = *buf - '0';
		printf("%c\n",*buf++);
	}
	
	printf("%d\n",read(newfd, qw, 3));
	printf("%c",qw);

	close(fd);
	return 0;
}
