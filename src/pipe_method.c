#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "pipe_method.h"


static void SignalHandler(int nSigno)
{
    signal(nSigno, SignalHandler);
    switch(nSigno)
    {
    case SIGPIPE:
//        printf("Process will not exit\n");
        break;
    default:
        printf("%d signal unregister\n", nSigno);
        break;
    }
}
 
void InitSignalHandler()
{
    signal(SIGPIPE , &SignalHandler);

}

int fork_pipe_create(int *pipefd)
{
//	int fpid = fork();
	if(pipe(pipefd)==-1){
		perror("pipe create error.\n");
		exit(EXIT_FAILURE);
	}
        int fpid = fork();
	return fpid;
}

void pipe_write(int pipeid, char *buf, int len)
{
        int ret;
	if((ret = write(pipeid,buf,len)) != len){
            printf("pipe_write error.\n");
	    exit(EXIT_FAILURE);
	}
}
