/*
    Writed by Sun Diansen. 18,Nov,2017.
*/
#include "costmodel.h"

/** Statistic Objects **/
static blkcnt_t CallBack_Cnt_Clean, CallBack_Cnt_Dirty;
static blkcnt_t Evict_Cnt_Clean, Evict_Cnt_Dirty;

/* Possibility of CallBack the clean/dirty blocks */
static double PCB_Clean, PCB_Dirty;
static blkcnt_t WindowSize;


/** The relavant objs of evicted blocks array belonged the current window. **/
typedef struct
{
    off_t       blk_offset;
    unsigned    flag;
    int         isCallBack;
} WDItem, WDArray;

static WDArray * WindowArray;
static blkcnt_t WDArray_Head, WDArray_Tail;

#define IsFull_WDArray() ( WDArray_Head == (WDArray_Tail + 1) % WindowSize )

/** The relavant objs of the Hash Index of the array **/
typedef off_t       hashkey_t;
typedef blkcnt_t    hashvalue_t;
typedef struct WDBucket
{
    off_t       key;
    blkcnt_t    itemId;
    struct WDBucket*  next;
} WDBucket;

static WDBucket * WDBucketPool, * WDBucket_Top;
static int WDPush_Bucket();
static WDBucket * WDPop_Bucket();

static WDBucket ** HashIndex;
static blkcnt_t hashGet_WDItemId(off_t key);
static int      hashInsert_WDItem(WDItem * item);
static int      hashRemove_WDItem(WDItem * item);
#define HashMap_KeytoValue(key) ((key / BLCKSZ) % WindowSize)

/** MAIN FUNCTIONS **/
int CM_Init(blkcnt_t windowSize)
{
    WindowSize = windowSize; // 5% cache block size for evicted blocks.
    WindowArray = (WDArray *)malloc(WindowSize * sizeof(WDItem));
    WDArray_Head = WDArray_Tail = 0;

    WDBucketPool = (WDBucket *)malloc(WindowSize * sizeof(WDBucket));
    WDBucket_Top = WDBucketPool;

    blkcnt_t n = 0;
    for(n = 0; n < WindowSize; n++)
    {
        WDBucketPool[n].next = WDBucketPool + n + 1;
    }
    WDBucketPool[WindowSize - 1].next = NULL;

    HashIndex = (WDBucket **)malloc(WindowSize * sizeof(WDItem*));
}

int CM_Reg_EvictBlk(SSDBufTag blktag, unsigned flag)
{

}

/* brief
 * return:
 * 0 : Clean BLock
 * 1 : Dirty Block
 */
int CM_CHOOSE()
{
    return 0;
}

/** Utilities **/
static int push_WDBucket(WDBucket * freeBucket)
{
    freeBucket->next = WDBucket_Top;
    WDBucket_Top = freeBucket;
    return 0;
}

static WDBucket * pop_WDBucket()
{
    WDBucket * bucket = WDBucket_Top;
    WDBucket_Top = bucket->next;
    bucket->next = NULL;
    return bucket;
}

static regIndex(off_t key)
{
    WDBucket * bucket = pop_WDBucket();
}

static int insertItem(off_t offset, unsigned flag)
{
    if(IsFull_WDArray())
    {
        /* clean the oldest block(the head block) information */
    }

    WDItem* tail = WindowArray + WDArray_Tail;
    tail->blk_offset = offset;
    tail->flag = flag;
    tail->isCallBack = 0;
    WDArray_Tail = (WDArray_Tail + 1) % WindowSize;


}


/** Utilities of hash index **/
static blkcnt_t hashGet_WDItemId(hashkey_t key)
{
    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket * bucket = HashIndex[hashcode];

    while(bucket != NULL)
    {
        if(bucket->key == key)
        {
            return bucket->itemId;
        }
        bucket = bucket->next;
    }
    return -1;
}

static int hashInsert_WDItem(hashkey_t key, hashvalue_t value)
{
    WDBucket* newBucket = pop_WDBucket();
    newBucket->key = key;
    newBucket->itemId = value;
    newBucket->next = NULL;

    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket* bucket = HashIndex[hashcode];

    if(bucket != NULL)
    {
        newBucket->next = bucket;
    }

    HashIndex[hashcode] = &newBucket;
    return 0;
}

static int hashRemove_WDItem(hashkey_t key)
{
    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket * bucket = HashIndex[hashcode];

    if(bucket == NULL)
    {
        error("[CostModel]: Trying to remove unexisted hash index!\n");
        exit(-1);
    }

    if(bucket->key == key)
    {
        HashIndex[hashcode] = bucket->next;
        push_WDBucket(bucket);
        return 0;
    }

    WDBucket * next_bucket = bucket->next;
    while(next_bucket != NULL)
    {
        if(next_bucket->key == key){
            bucket->next = next_bucket->next;
            push_WDBucket(next_bucket);
            return 0;
        }
        bucket = next_bucket;
        next_bucket = bucket->next;
    }

        error("[CostModel]: Trying to remove unexisted hash index!\n");
        exit(-1);
}

