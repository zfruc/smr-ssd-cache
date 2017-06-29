#include "global.h"

/** This user basic info */
int BatchId;
int UserId;
int TraceId;
off_t StartLBA;
int WriteOnly;
int BatchSize;
SSDEvictionStrategy EvictStrategy;

unsigned long Param1;
unsigned long Param2;

/** All users basic setup **/
blksize_t NBLOCK_SSD_CACHE;
blksize_t NTABLE_SSD_CACHE;
blksize_t SSD_BUFFER_SIZE = 4096;
blksize_t NSMRBands = 19418000;		// 194180*(18MB+36MB)/2~5TB
blksize_t NSMRBlocks = 2621952;		// 2621952*8KB~20GB
blksize_t NSSDs;
blksize_t NSSDTables;
blksize_t NBANDTables = 2621952;
blksize_t SSD_SIZE = 4096;
blksize_t BLCKSZ = 4096;
blksize_t BNDSZ = 36*1024*1024;
blksize_t ZONESZ;
blksize_t INTERVALTIMELIMIT = 100000000;
blksize_t NSSDLIMIT;
blksize_t NSSDCLEAN = 1;
blksize_t WRITEAMPLIFICATION = 100;
blksize_t NCOLDBAND = 1;
blksize_t PERIODTIMES;
char smr_device[] = "/dev/sdc";
char ssd_device[] = "/mnt/ssd/ssd";
char ram_device[1024];

int BandOrBlock;

/*Block = 0, Band=1*/
int hdd_fd;
int ssd_fd;
int ram_fd;
struct RuntimeSTAT* STT;

/** Shared memory variable names **/
const char* SHM_SSDBUF_STRATEGY_CTRL = "SHM_SSDBUF_STRATEGY_CTRL";
const char* SHM_SSDBUF_STRATEGY_DESP = "SHM_SSDBUF_STRATEGY_DESP";

const char* SHM_SSDBUF_DESP_CTRL = "SHM_SSDBUF_DESP_CTRL";
const char* SHM_SSDBUF_DESPS = "SHM_SSDBUF_DESPS";

const char* SHM_SSDBUF_HASHTABLE_CTRL = "SHM_SSDBUF_HASHTABLE_CTRL";
const char* SHM_SSDBUF_HASHTABLE = "SHM_SSDBUF_HASHTABLE";
const char* SHM_SSDBUF_HASHDESPS =  "SHM_SSDBUF_HASHDESPS";
const char* SHM_PROCESS_REQ_LOCK = "SHM_PROCESS_REQ_LOCK";

const char* PATH_LOG = "/home/outputs/logs";

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
