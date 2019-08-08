#include <unistd.h>
#include "pipe_method.h"

int fork_pipe_create(int *pipefd)
{
	int fpid = fork();
	if(pipe(pipefd)==-1){
		perror("pipe create error.\n");
		exit(EXIT_FAILURE);
	}
	return fpid;
}

void pipe_write(int pipeid, char *buf, int len)
{
	write(pipeid,buf,len);
}
