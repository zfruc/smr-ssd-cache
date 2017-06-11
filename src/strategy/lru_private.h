#define DEBUG 0
/* ---------------------------lru---------------------------- */

typedef struct
{
	long 		serial_id;			// the corresponding descriptor serial number.
    long        next_lru;               // to link used SSD as LRU
    long        last_lru;               // to link used SSD as LRU
    long        next_self_lru;
    long        last_self_lru;
    int   user_id;
    pthread_mutex_t lock;
} StrategyDesp_LRU_private;

typedef struct
{
    long        first_lru;          // Head of list of LRU
    long        last_lru;           // Tail of list of LRU
    pthread_mutex_t lock;
} StrategyCtrl_LRU_private;

extern int initSSDBufferFor_LRU_private();
extern long Unload_Buf_LRU_private();
extern int hitInBuffer_LRU_private(long serial_id);
extern void *insertBuffer_LRU_private(long serial_id);
