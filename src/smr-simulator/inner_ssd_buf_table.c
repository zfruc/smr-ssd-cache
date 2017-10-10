#include <stdio.h>
#include <stdlib.h>

#include "inner_ssd_buf_table.h"
#include "smr-simulator/simulator_v2.h"
#include "cache.h"

static SSDHashBucket* HashTable;
#define GetSSDHashBucket(hash_code) ((SSDHashBucket *) (HashTable + (unsigned long) (hash_code)))

#define IsSame(T1, T2) ((T1.offset == T2.offset) ? 1 : 0)

void initSSDTable(size_t size)
{
	HashTable = (SSDHashBucket*)malloc(sizeof(SSDHashBucket)*size);
	size_t i;
	SSDHashBucket *bucket = HashTable;
	for (i = 0; i < size; bucket++, i++){
		bucket->despId = -1;
		bucket->hash_key.offset = -1;
		bucket->next_item = NULL;
	}
}

unsigned long ssdtableHashcode(DespTag tag)
{
	unsigned long ssd_hash = (tag.offset / SSD_BUFFER_SIZE) % NBLOCK_SMR_FIFO;
	return ssd_hash;
}

long ssdtableLookup(DespTag tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Lookup tag: %lu\n",tag.offset);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	while (nowbucket != NULL) {
	//	printf("nowbucket->buf_id = %u %u %u\n", nowbucket->hash_key.rel.database, nowbucket->hash_key.rel.relation, nowbucket->hash_key.block_num);
		if (IsSame(nowbucket->hash_key, tag)) {
	//		printf("find\n");
			return nowbucket->despId;
		}
		nowbucket = nowbucket->next_item;
	}
//	printf("no find\n");

	return -1;
}

long ssdtableInsert(DespTag tag, unsigned long hash_code, long despId)
{
	if (DEBUG)
		printf("[INFO] Insert tag: %lu, hash_code=%lu\n",tag.offset, hash_code);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	while (nowbucket->next_item != NULL) {
		nowbucket = nowbucket->next_item;
	}
    static void* itembuf;
    int ret = posix_memalign(&itembuf,512,sizeof(SSDHashBucket)); 
    SSDHashBucket *newitem = (SSDHashBucket*)itembuf;
    newitem->hash_key = tag;
    newitem->despId = despId;
    newitem->next_item = NULL;
    nowbucket->next_item = newitem;

	return 0;
}

long ssdtableDelete(DespTag tag, unsigned long hash_code)
{
	if (DEBUG)
		printf("[INFO] Delete tag: %lu, hash_code=%lu\n",tag.offset, hash_code);
	SSDHashBucket *nowbucket = GetSSDHashBucket(hash_code);
	long del_id;
	SSDHashBucket *delitem;

	while (nowbucket->next_item != NULL) {
		if (IsSame(nowbucket->next_item->hash_key, tag)) {
            delitem = nowbucket->next_item;
            del_id = delitem->despId;
            nowbucket->next_item = delitem->next_item;
            free(delitem);
            return del_id;
		}
		nowbucket = nowbucket->next_item;
	}

	return -1;
}

long ssdtableUpdate(DespTag tag, unsigned long hash_code, long despId)
{
	if (DEBUG)
		printf("[INFO] Insert tag: %lu, hash_code=%lu\n",tag.offset, hash_code);
	SSDHashBucket* nowbucket = GetSSDHashBucket(hash_code);
	SSDHashBucket* lastbucket = nowbucket;
	while (nowbucket != NULL) {
        lastbucket = nowbucket;
        if (IsSame(nowbucket->hash_key,tag)) {
            long oldId = nowbucket->despId;
            nowbucket->despId = despId;
            return oldId;
		}
		nowbucket = nowbucket->next_item;
	}

	// if not exist in table, insert one.
    static void* itembuf;
    int ret = posix_memalign(&itembuf,512,sizeof(SSDHashBucket));
    SSDHashBucket *newitem = (SSDHashBucket*)itembuf;
    newitem->hash_key = tag;
    newitem->despId = despId;
    newitem->next_item = NULL;
    lastbucket->next_item = newitem;
	return -1;
}
