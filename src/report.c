#include <stdio.h>
#include <errno.h>
FILE* LogFile;
void* info(char* str)
{
    printf("process [%d]: %s\n",getpid(),str);
}

void* error(char* str)
{
    printf("process [%d]: %s\n",getpid(),str);
    exit(1);
}

int OpenLogFile(const char* filepath)
{
    LogFile = fopen(filepath,"w+");
    if(LogFile == NULL)
        return errno;
}

int CloseLogFile()
{
    return fclose(LogFile);
}
int WriteLog(char* log)
{
    return fwrite(log,strlen(log),1,LogFile);
}
