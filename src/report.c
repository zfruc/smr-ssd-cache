void* info(char* str)
{
    printf("process [%d]: %s\n",getpid(),str);
}

void* error(char* str)
{
    printf("process [%d]: %s\n",getpid(),str);
}
