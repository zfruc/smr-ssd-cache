/*
    Write by Sun Diansen.
    This file provides utilities for share memory operation based on POSIX Share Memory.
    To untilize without error, you might add "-lrt" option onto the linker.
*/

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

/*
    Only call this func one time, otherwise it may cause content confused.
*/
void* SHMalloc(char* shm_name,size_t len)
{
    int fd = shm_open(shm_name, O_RDWR|O_CREAT,0644);
    if(fd < 0)
    {
        printf("%s","create share memory error.\n");

    }

    if(ftruncate(fd,len)!=0)
    {
        printf("%s","Truncate share memory error.\n");
        shm_unlink(shm_name);
        return NULL;
    }
    void* shm_vir_addr = mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    return shm_vir_addr;
}

int SHMfree(char* shm_name,void* shm_vir_addr,long len)
{
    if(munmap(shm_vir_addr,len)<0 || shm_unlink(shm_name)<0)
        return -1;
    return 0;
}

void* getSHM(char* shm_name,size_t len)
{
    int fd = shm_open(shm_name,O_RDWR,0644);
    if(fd < 0)
        return NULL;
    return mmap(NULL,len,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
}
