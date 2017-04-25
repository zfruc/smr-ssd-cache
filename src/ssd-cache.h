#define DEBUG 0
#define GLOBLE_STRATEGY 1

/* ---------------------------ssd cache---------------------------- */

#include <pthread.h>
//#define size_t	unsigned long
#define off_t	unsigned long
#define bool	unsigned char

typedef struct
{
	off_t	offset;
} SSDBufferTag;

typedef struct
{
	SSDBufferTag 	ssd_buf_tag;
	long 		ssd_buf_id;				// ssd buffer location in shared buffer
	unsigned 	ssd_buf_flag;
	long		next_freessd;           // to link free ssd
} SSDBufDesp;

#define SSD_BUF_VALID 0x01
#define SSD_BUF_DIRTY 0x02

typedef struct SSDBufHashCtrl
{
    pthread_mutex_t lock;
}SSDBufHashCtrl;

typedef struct SSDBufHashBucket
{
	SSDBufferTag 			hash_key;
	long    				ssd_buf_id;
	struct SSDBufHashBucket 	*next_item;
} SSDBufHashBucket;

typedef struct
{
	long		n_usedssd;			// For eviction
	long		first_freessd;		// Head of list of free ssds
	long		last_freessd;		// Tail of list of free ssds
	pthread_mutex_t lock;
} SSDBufDespCtrl;

typedef enum
{
	CLOCK = 0,
    LRU,
	LRUofBand,
	Most,
	Most_Dirty,
	SCAN,
	CMR,
	SMR,
    WA,
    MaxCold,
    MaxAll,
    AvgBandHot,
    HotDivSize,
    /** add for multiuser **/
    LRU_global,
    LRU_peruser
} SSDEvictionStrategy;

extern size_t BNDSZ;
extern int BandOrBlock;

extern SSDBufHashCtrl   *ssd_buf_hash_ctrl;
extern SSDBufHashBucket	*ssd_buf_hashtable;
extern SSDBufHashBucket *ssd_buf_hashdesps;

extern SSDBufDespCtrl *ssd_buf_desp_ctrl;
extern SSDBufDesp	*ssd_buf_desps;

extern pthread_mutex_t  *lock_process_req;

extern unsigned long hit_num;
extern unsigned long flush_ssd_blocks;
extern unsigned long read_ssd_blocks;
extern unsigned long miss_hashitem_num;
extern double time_read_cmr;
extern double time_write_cmr;
extern double time_read_ssd;
extern double time_write_ssd;

extern unsigned long read_hit_num;
//extern unsigned long write-ssd-num;
//extern unsigned long flush_times;

#define GetSSDBufHashBucket(hash_code) ((SSDBufHashBucket *) (ssd_buf_hashtable + (unsigned) (hash_code)))

extern void initSSD();
extern void read_block(off_t offset, char* ssd_buffer);
extern void write_block(off_t offset,int blkcnt, char* ssd_buffer);
extern void read_band(off_t offset, char* ssd_buffer);
extern void write_band(off_t offset, char* ssd_buffer);

extern void CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag);

//extern int read(unsigned offset);
//extern int write(unsigned offset);
extern void* flushSSDBuffer(SSDBufDesp *ssd_buf_hdr);

extern unsigned long NBLOCK_SSD_CACHE;
extern unsigned long NTABLE_SSD_CACHE;
extern size_t SSD_BUFFER_SIZE;
extern char	smr_device[100];
extern int 	smr_fd;
extern int 	ssd_fd;
extern SSDEvictionStrategy EvictStrategy;

extern const char* SHM_SSDBUF_STRATEGY_CTRL;
extern const char* SHM_SSDBUF_STRATEGY_DESP;
extern const char* SHM_SSDBUF_DESP_CTRL;
extern const char* SHM_SSDBUF_DESPS;
extern const char* SHM_SSDBUF_HASHTABLE_CTRL;
extern const char* SHM_SSDBUF_HASHTABLE;
extern const char* SHM_SSDBUF_HASHDESPS;
extern const char* SHM_PROCESS_REQ_LOCK;
