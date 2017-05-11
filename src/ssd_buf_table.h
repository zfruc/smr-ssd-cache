#ifndef _SSDBUFTABLE_H
#define _SSDBUFTABLE_H 1

#include "ssd-cache.h"

extern int HashTab_Init();
extern unsigned long HashTab_GetHashCode(SSDBufferTag *ssd_buf_tag);
extern long HashTab_Lookup(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
extern long HashTab_Insert(SSDBufferTag *ssd_buf_tag, unsigned long hash_code, size_t desp_serial_id);
extern long HashTab_Delete(SSDBufferTag *ssd_buf_tag, unsigned long hash_code);
#endif   /* SSDBUFTABLE_H */
