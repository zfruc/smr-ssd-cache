#define DEBUG 0
/* ---------------------------lru---------------------------- */

typedef struct
{
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
    long        next_lru;               // to link used ssd as LRU
    long        last_lru;               // to link used ssd as LRU
} SSDBufDespForLRU;

typedef struct
{
    long        first_lru;          // Head of list of LRU
    long        last_lru;           // Tail of list of LRU
    pthread_mutex_t lock;
} SSDBufferStrategyControlForLRU;

/********
 ** SHM**
 ********/
SSDBufferStrategyControlForLRU *ssd_buf_strategy_ctrl_lru;
SSDBufDespForLRU	*ssd_buf_desp_for_lru;
extern unsigned long flush_times;


extern int initSSDBufferForLRU();
extern SSDBufDesp *getLRUBuffer();
extern void *hitInLRUBuffer(SSDBufDesp *);
