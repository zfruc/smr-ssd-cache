#ifndef SSDBUFTABLE_H
#define SSDBUFTABLE_H
#include "smr-simulator/smr-simulator.h"
#include "smr-simulator/simulator_logfifo.h"

typedef struct SSDHashBucket
{
        DespTag	hash_key;
        long    despId;
        struct SSDHashBucket *next_item;
} SSDHashBucket;

extern void initSSDTable(size_t size);
extern unsigned long ssdtableHashcode(DespTag *tag);
extern long ssdtableLookup(DespTag *tag, unsigned long hash_code);
extern long ssdtableInsert(DespTag *tag, unsigned long hash_code, long despId);
extern long ssdtableDelete(DespTag *tag, unsigned long hash_code);
#endif   /* SSDBUFTABLE_H */
