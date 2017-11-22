/*
    Writed by Sun Diansen. 18,Nov,2017.

*/
#include "costmodel.h"

/** Statistic Objects **/
static blkcnt_t CallBack_Cnt_Clean, CallBack_Cnt_Dirty;
static blkcnt_t Evict_Cnt_Clean, Evict_Cnt_Dirty;

/* RELATED PARAMETER OF COST MODEL */
static double   CC, CD;                 /* Cost of Clean/Dirty blocks. */
static double   PCB_Clean, PCB_Dirty;   /* Possibility of CallBack the clean/dirty blocks */
static double   WtrAmp;                 /* The write amp of blocks i'm going to evict. */

/* time of random, sequence, into fifo, average.
 * And each of these collects from different way, such as:
 * T_rand from when calling the 'CM_CallBack' func and sending a parameter of read block time.
 * T_seq is the inner-disk time of sequenced IO, which depends on the hardware parameter (Set 40 microsecond).
 * T_fifo should be a hardware parameter, but it's inner metadata management can't be seen, so I mearsure it from evicted dirty block.
 * T_avg is the average time cost, which is (dirty evicted time sum / evicted count)
 */
static int  T_rand, T_fifo, T_avg, T_seq = 40;
static microsecond_t T_rand_sum = 0;
static microsecond_t T_fifo_sum = 0;
static blkcnt_t T_rand_cnt = 0;
static blkcnt_t T_fifo_cnt = 0;
static blkcnt_t T_evict_cnt = 0;
static microsecond_t * t_randcollect;

/** The relavant objs of evicted blocks array belonged the current window. **/
static blkcnt_t WindowSize;
typedef struct
{
    off_t       offset;
    unsigned    flag;
    microsecond_t usetime;
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
static int          push_WDBucket();
static WDBucket *   pop_WDBucket();

static WDBucket ** HashIndex;
static blkcnt_t indexGet_WDItemId(off_t key);
static int      indexInsert_WDItem(hashkey_t key, hashvalue_t value);
static int      indexRemove_WDItem(hashkey_t key);
#define HashMap_KeytoValue(key) ((key / BLCKSZ) % WindowSize)

/** MAIN FUNCTIONS **/
int CM_Init(blkcnt_t windowSize)
{
    WindowSize = windowSize;
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

    HashIndex = (WDBucket **)calloc(WindowSize, sizeof(WDItem*));
    CallBack_Cnt_Clean = CallBack_Cnt_Dirty = Evict_Cnt_Clean = Evict_Cnt_Dirty = 0;

    t_randcollect = (microsecond_t *)malloc(WindowSize * sizeof(microsecond_t));

    if(WindowArray == NULL ||WDBucketPool == NULL || HashIndex == NULL || t_randcollect == NULL)
        return -1;
    return 0;
}

/* To register a evicted block metadata.
 * But notice that, it can not be register a block has been exist in the window array.
 * Which means you can not evict block who has been evicted a short time ago without a call back.
 * If you do the mistake, it will cause the error of metadata managment. Because I didn't set consistency checking mechanism.
 */
int CM_Reg_EvictBlk(SSDBufTag blktag, unsigned flag, microsecond_t usetime)
{
    if(IsFull_WDArray())
    {
        /* Clean all the information related to the oldest block(the head block)  */
        WDItem* oldestItem = WindowArray +  WDArray_Head;

        // clean the index.
        if(!oldestItem->isCallBack)
        {
            indexRemove_WDItem(oldestItem->offset);
        }

        // 2. clean the metadata
        T_evict_cnt --;
        if((oldestItem->flag & SSD_BUF_DIRTY) != 0)
        {
            T_fifo_cnt --;
            T_fifo_sum -= oldestItem->usetime;

            Evict_Cnt_Dirty --;
            if(oldestItem->isCallBack)
                CallBack_Cnt_Dirty --;
        }
        else
        {
            Evict_Cnt_Clean --;
            if(oldestItem->isCallBack)
                CallBack_Cnt_Clean--;
        }

        // 3. retrieve WDItem
        WDArray_Head = (WDArray_Head + 1) % WindowSize;
    }

    // Add a new one to the tail.
    WDItem* tail = WindowArray + WDArray_Tail;
    tail->offset = blktag.offset;
    tail->flag = flag;
    tail->usetime = usetime;
    tail->isCallBack = 0;

    indexInsert_WDItem(tail->offset, WDArray_Tail);
    WDArray_Tail = (WDArray_Tail + 1) % WindowSize;

    if((flag & SSD_BUF_DIRTY) != 0)
    {
        Evict_Cnt_Dirty ++;
        T_fifo_cnt ++;
        T_fifo_sum += usetime;
    }
    else
        Evict_Cnt_Clean ++;
    T_evict_cnt ++;

    return 0;
}

/* Check if it exist in the evicted array within the window.
 * If exist, then to mark 'callback',
 * and remove it's index to avoid the incorrectness when next time callback.
 */
int CM_TryCallBack(SSDBufTag blktag)
{
    hashkey_t key = blktag.offset;
    blkcnt_t itemId = indexGet_WDItemId(key);
    if(itemId >= 0)
    {
        WDItem* item = WindowArray + itemId;
        item->isCallBack = 1;
        indexRemove_WDItem(key);
        if((item->flag & SSD_BUF_DIRTY) != 0)
            CallBack_Cnt_Dirty ++;
        else
            CallBack_Cnt_Clean ++;
        return 1;
    }
    return 0;
}

/* brief
 * return:
 * 0 : Clean BLock
 * 1 : Dirty Block
 */
int CM_CHOOSE(cm_token token)
{
    static blkcnt_t counter = 0;
    counter ++ ;

    if(counter % 1000 == 0)
        ReportCM();
    PCB_Clean = (double)CallBack_Cnt_Clean / Evict_Cnt_Clean;
    PCB_Dirty = (double)CallBack_Cnt_Dirty / Evict_Cnt_Dirty;

    int clean_cnt = token.will_evict_clean_blkcnt;
    int dirty_cnt = token.will_evict_dirty_blkcnt;
    WtrAmp = token.wtramp;

    T_rand = T_rand_sum / T_rand_cnt;
    T_fifo = T_fifo_sum / T_fifo_cnt;
    T_avg = T_fifo_sum / T_evict_cnt;

    CC = (0 + PCB_Clean * (T_rand + T_avg)) * clean_cnt;
    CD = (T_fifo + PCB_Dirty * T_avg + WtrAmp * T_seq) * dirty_cnt;


    return CC < CD ? 0 : 1;
}

int CM_T_rand_Reg(microsecond_t usetime)
{
    static blkcnt_t head = 0, tail = 0;
    if(head == (tail + 1) % WindowSize)
    {
        // full
        T_rand_sum -= t_randcollect[head];
        T_rand_cnt --;
        head = (head + 1) % WindowSize;
    }

    T_rand_sum += usetime;
    T_rand_cnt ++;
    t_randcollect[tail] = usetime;
    tail = (tail + 1) % WindowSize;

    return 0;
}

void ReportCM()
{
    printf("---------- Cost Model Runtime Report ----------\n");
    printf("current evited block count: clean / dirty:");
    printf("%ld / %ld\n", Evict_Cnt_Clean, Evict_Cnt_Dirty);

    printf("current possibility of call-back: clean / dirty:\n");
    printf("%.2f\% / %.2f\%\n", PCB_Clean*100, PCB_Dirty*100);

    printf("T_rand,\tT_fifo,\tT_avg,\tT_seq\n");
    printf("%d,\t%d,\t%d,\t%d\n", T_rand, T_fifo, T_avg, T_seq);

    printf("Cost of evict clean blocks (0 + PCB_Clean * (T_rand + T_avg))\n");
    printf("%.2f * clean_cnt\n", PCB_Clean * (T_rand + T_avg));
    printf("Cost of evict dirty blocks ((T_fifo + PCB_Dirty * T_avg + WtrAmp * T_seq) * dirty_cnt)\n");
    printf("%.2f * dirty_cnt\n", T_fifo + PCB_Dirty * T_avg + WtrAmp * T_seq);

}
/** Utilities of Hash Index **/
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

static blkcnt_t indexGet_WDItemId(hashkey_t key)
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

static int indexInsert_WDItem(hashkey_t key, hashvalue_t value)
{
    if(key == 140735284549392)
    {
        int a = 1;
    }
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

    HashIndex[hashcode] = newBucket;
    return 0;
}

static int indexRemove_WDItem(hashkey_t key)
{
    blkcnt_t hashcode = HashMap_KeytoValue(key);
    WDBucket * bucket = HashIndex[hashcode];

    if(bucket == NULL)
    {
        error("[CostModel]: Trying to remove unexisted hash index!\n");
        exit(-1);
    }

    // if is the first bucket
    if(bucket->key == key)
    {
        HashIndex[hashcode] = bucket->next;
        push_WDBucket(bucket);
        return 0;
    }

    // else
    WDBucket * next_bucket = bucket->next;
    while(next_bucket != NULL)
    {
        if(next_bucket->key == key)
        {
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

