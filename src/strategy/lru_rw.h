#ifndef _LRU_RW_H_
#define _LRU_RW_H_
#define DEBUG 0

#include "lru_private.h"

extern int initSSDBufferFor_LRU_rw();
extern long Unload_Buf_LRU_rw(unsigned flag);
extern int hitInBuffer_LRU_rw(long serial_id, unsigned flag);
extern int insertBuffer_LRU_rw(long serial_id, unsigned flag);
#endif // _LRU_PRIVATE_H_

