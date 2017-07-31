#ifndef _GLOBAL_H
#define _GLOBAL_H 1

#include <sys/types.h>


struct RuntimeSTAT
{
    /** This user basic info */
    unsigned int batchId;
    unsigned int userId;
    unsigned int traceId;
    unsigned long startLBA;
    unsigned int isWriteOnly;

    /** Runtime strategy refered parameter **/
    //union StratetyUnion strategyRef;

    /** Runtime Statistic **/
    blksize_t cacheLimit;
    blksize_t cacheUsage;

    blksize_t reqcnt_s;
    blksize_t reqcnt_r;
    blksize_t reqcnt_w;

    blksize_t hitnum_s;
    blksize_t hitnum_r;
    blksize_t hitnum_w;

    blksize_t load_ssd_blocks;
    blksize_t load_hdd_blocks;
    blksize_t flush_hdd_blocks;
    blksize_t flush_ssd_blocks;
    blksize_t flush_clean_blocks;

    double time_read_ssd;
    double time_read_hdd;
    double time_write_ssd;
    double time_write_hdd;

    blksize_t hashmiss_sum;
    blksize_t hashmiss_read;
    blksize_t hashmiss_write;
};

typedef enum
{
//    CLOCK = 0,
    LRU,
    LRUofBand,
//    Most,
//    Most_Dirty,
//    SCAN,
//    CMR,
//    SMR,
//    WA,
//    MaxCold,
//    MaxAll,
//    AvgBandHot,
//    HotDivSize,
    /** add for multiuser **/
    LRU_global,
    LRU_private,
    LRU_batch,
    PORE
}SSDEvictionStrategy;

/** This user basic info */
extern int BatchId;
extern int UserId;
extern int TraceId;
extern off_t StartLBA;
extern int WriteOnly;
extern SSDEvictionStrategy EvictStrategy;
extern unsigned long Param1;
extern unsigned long Param2;
extern int BatchSize;

/** All users basic setup **/
extern blksize_t NBLOCK_SSD_CACHE;
extern blksize_t NTABLE_SSD_CACHE;
extern blksize_t SSD_BUFFER_SIZE;
extern blksize_t NBLOCK_SMR_FIFO;
//extern blksize_t NSMRBands;		// 194180*(18MB+36MB)/2~5TB
//extern blksize_t NSMRBlocks;		// 2621952*8KB~20GB
//extern blksize_t NSSDs;
extern blksize_t NSSDTables;
extern blksize_t NBANDTables;
extern blksize_t SSD_SIZE;
extern blksize_t BLCKSZ;
extern blksize_t BNDSZ;
extern blksize_t NZONES;
extern blksize_t ZONESZ;
extern blksize_t INTERVALTIMELIMIT;
extern blksize_t NSSDLIMIT;
extern blksize_t NSSDCLEAN;
extern blksize_t WRITEAMPLIFICATION;
extern blksize_t NCOLDBAND;
extern blksize_t PERIODTIMES;
extern char smr_device[];
extern char ssd_device[];
extern char ram_device[1024];

extern int BandOrBlock;

/*Block = 0, Band=1*/
extern int hdd_fd;
extern int ssd_fd;
extern int ram_fd;
extern struct RuntimeSTAT* STT;

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
blksize_t interval_time;
blksize_t read_smr_bands;
blksize_t flush_bands;
blksize_t flush_band_size;

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
