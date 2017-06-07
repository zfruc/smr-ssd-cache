#include "global.h"

unsigned long NBLOCK_SSD_CACHE;
unsigned long NTABLE_SSD_CACHE;
unsigned long SSD_BUFFER_SIZE;
unsigned long NSMRBands = 19418000;		// 194180*(18MB+36MB)/2~5TB
unsigned long NSMRBlocks = 2621952;		// 2621952*8KB~20GB
unsigned long NSSDs;
unsigned long NSSDTables;
unsigned long NBANDTables = 2621952;
size_t SSD_SIZE = 4096;
size_t BLCKSZ = 4096;
size_t BNDSZ = 36*1024*1024;
size_t ZONESZ;
unsigned long INTERVALTIMELIMIT = 100000000;
unsigned long NSSDLIMIT;
unsigned long NSSDCLEAN = 1;
unsigned long WRITEAMPLIFICATION = 100;
unsigned long NCOLDBAND = 1;
unsigned long PERIODTIMES;
char smr_device[] = "/dev/sdc";
char ssd_device[] = "/mnt/ramdisk/ssd";

SSDEvictionStrategy EvictStrategy;

int BandOrBlock;

/*Block = 0, Band=1*/
int hdd_fd;
int ssd_fd;

struct RuntimeSTAT* STT;
struct InitUsrInfo UsrInfo;

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
const char* SHM_SSDBUF_STRATEGY_CTRL = "SHM_SSDBUF_STRATEGY_CTRL";
const char* SHM_SSDBUF_STRATEGY_DESP = "SHM_SSDBUF_STRATEGY_DESP";

const char* SHM_SSDBUF_DESP_CTRL = "SHM_SSDBUF_DESP_CTRL";
const char* SHM_SSDBUF_DESPS = "SHM_SSDBUF_DESPS";

const char* SHM_SSDBUF_HASHTABLE_CTRL = "SHM_SSDBUF_HASHTABLE_CTRL";
const char* SHM_SSDBUF_HASHTABLE = "SHM_SSDBUF_HASHTABLE";
const char* SHM_SSDBUF_HASHDESPS =  "SHM_SSDBUF_HASHDESPS";
const char* SHM_PROCESS_REQ_LOCK = "SHM_PROCESS_REQ_LOCK";

const char* PATH_LOG = "/home/outputs/logs";
