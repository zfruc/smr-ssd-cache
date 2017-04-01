#ifndef SHMLIB
#define SHMLIB

extern void* SHMalloc(char* shm_name,size_t len);
extern int SHMfree(char* shm_name,void* shm_vir_addr,long len);
extern void* getSHM(char* shm_name,size_t len);

#endif
