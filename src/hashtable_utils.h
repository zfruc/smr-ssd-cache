#ifndef _SSDBUFTABLE_H
#define _SSDBUFTABLE_H 1

#include "cache.h"

#define GetSSDBufHashBucket(hash_code) ((SSDBufHashBucket *) (ssd_buf_hashtable + (unsigned) (hash_code)))

typedef struct SSDBufHashBucket
{
    SSDBufferTag 			hash_key;
    long    				desp_serial_id;
    struct SSDBufHashBucket 	*next_item;
} SSDBufHashBucket;

extern SSDBufHashBucket* ssd_buf_hashtable;
extern int HashTab_Init();
extern unsigned long HashTab_GetHashCode(SSDBufferTag *ssd_buf_tag);
extern long HashTab_Lookup(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
extern long HashTab_Insert(SSDBufferTag *ssd_buf_tag, unsigned long hash_code, long desp_serial_id);
extern long HashTab_Delete(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
#endif   /* SSDBUFTABLE_H */
