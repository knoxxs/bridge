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
	char* buf,*temp,str[5];

	int fd = open("file", O_RDWR | O_CREAT , 0644);
	int newfd = open("bitch", O_RDWR | O_CREAT | O_APPEND, 0644);
	if(fstat(fd,&sb));

	int *ptr;

	ptr = mmap(NULL,sb.st_size, PROT_READ|PROT_WRITE, MAP_SHARED,fd,(off_t)0);
	if (ptr == MAP_FAILED) {
		close(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}
	
	int i = 0;
	buf = (char *)ptr;
	printf("%d\n",sb.st_size);
	temp =(char *) ptr;
	sprintf(str,"%d",newfd);
	printf("%s\n",str);
	write(fd, str,strlen(str));
	fstat(fd,&sb);
	printf("%d\n",sb.st_size);
	while(i++<sb.st_size)
	{
		printf("%c\n",*buf++);
	}
close(fd);
	return 0;
}

