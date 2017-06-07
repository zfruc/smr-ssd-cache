#ifndef _GLOBAL_H
#define _GLOBAL_H 1
#include <sys/types.h>
typedef unsigned long size_t;

struct RuntimeSTAT
{
    unsigned int batchId;
    unsigned int userId;
    unsigned int traceId;
    unsigned int startLBA;
    unsigned int isWriteOnly;

    unsigned long reqcnt_s;
    unsigned long reqcnt_r;
    unsigned long reqcnt_w;

    unsigned long hitnum_s;
    unsigned long hitnum_r;
    unsigned long hitnum_w;

    unsigned long load_ssd_blocks;
    unsigned long load_hdd_blocks;
    unsigned long flush_hdd_blocks;
    unsigned long flush_ssd_blocks;
    unsigned long flush_clean_blocks;

    double time_read_ssd;
    double time_read_hdd;
    double time_write_ssd;
    double time_write_hdd;

    unsigned long hashmiss_sum;
    unsigned long hashmiss_read;
    unsigned long hashmiss_write;
};

struct InitUsrInfo
{
    int isWriteOnly;
    int traceId;
    off_t startLBA;
    int batchId;
    int usrId;
};

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


extern unsigned long NBLOCK_SSD_CACHE;
extern unsigned long NTABLE_SSD_CACHE;
extern unsigned long SSD_BUFFER_SIZE;
extern unsigned long NSMRBands;		// 194180*(18MB+36MB)/2~5TB
extern unsigned long NSMRBlocks;		// 2621952*8KB~20GB
extern unsigned long NSSDs;
extern unsigned long NSSDTables;
extern unsigned long NBANDTables;
extern size_t SSD_SIZE;
extern size_t BLCKSZ;
extern size_t BNDSZ;
extern size_t ZONESZ;
extern unsigned long INTERVALTIMELIMIT;
extern unsigned long NSSDLIMIT;
extern unsigned long NSSDCLEAN;
extern unsigned long WRITEAMPLIFICATION;
extern unsigned long NCOLDBAND;
extern unsigned long PERIODTIMES;
extern char smr_device[];
extern char ssd_device[];

extern SSDEvictionStrategy EvictStrategy;
extern int BandOrBlock;

/*Block = 0, Band=1*/
extern int hdd_fd;
extern int ssd_fd;

extern struct RuntimeSTAT* STT;
extern struct InitUsrInfo UsrInfo;

//extern SSDBufDespCtrl	*ssd_buf_desp_ctrl;
//extern SSDBufDesp	    *ssd_buf_desps;

//SSDBufHashCtrl   *ssd_buf_hash_ctrl;
//extern SSDBufHashBucket *ssd_buf_hashtable;
//SSDBufHashBucket *ssd_buf_hashdesps;

#ifdef SIMULATION
int 		    inner_ssd_fd;
SSDStrategyControl	*ssd_strategy_control;
SSDDesc		*ssd_descriptors;
SSDHashBucket	*ssd_hashtable;
unsigned long interval_time;
unsigned long read_smr_bands;
unsigned long flush_bands;
unsigned long flush_band_size;

pthread_mutex_t free_ssd_mutex;
pthread_mutex_t inner_ssd_hdr_mutex;
pthread_mutex_t inner_ssd_hash_mutex;
#endif // SIMULATION

/** Shared memory variable names **/
extern const char* SHM_SSDBUF_STRATEGY_CTRL;
extern const char* SHM_SSDBUF_STRATEGY_DESP;

extern const char* SHM_SSDBUF_DESP_CTRL;
extern const char* SHM_SSDBUF_DESPS;

extern const char* SHM_SSDBUF_HASHTABLE_CTRL;
extern const char* SHM_SSDBUF_HASHTABLE;
extern const char* SHM_SSDBUF_HASHDESPS;
extern const char* SHM_PROCESS_REQ_LOCK;

extern const char* PATH_LOG;
#endif
