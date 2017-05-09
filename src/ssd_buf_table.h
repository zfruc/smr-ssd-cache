#ifndef _SSDBUFTABLE_H
#define _SSDBUFTABLE_H 1

#include "ssd-cache.h"

extern int initSSDBufTable(size_t size);
extern unsigned long ssdbuftableHashcode(SSDBufferTag *ssd_buf_tag);
extern long ssdbuftableLookup(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
extern long ssdbuftableInsert(SSDBufferTag *ssd_buf_tag, unsigned long hash_code, size_t ssd_buf_id);
extern long ssdbuftableDelete(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
#endif   /* SSDBUFTABLE_H */
