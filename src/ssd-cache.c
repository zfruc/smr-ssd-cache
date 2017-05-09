#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "ssd_buf_table.h"
#include "strategy/clock.h"
#include "strategy/lru.h"
#include "strategy/lruofband.h"
#include "strategy/most.h"
#include "strategy/scan.h"
#include "strategy/WA.h"
#include "strategy/maxcold.h"
#include <shmlib.h>
void CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag);
static SSDBufDesp *SSDBufferAlloc(SSDBufferTag *ssd_buf_tag, bool * found, int alloc4What);

static int initStrategySSDBuffer(SSDEvictionStrategy strategy);
static SSDBufDesp *getSSDStrategyBuffer(SSDBufferTag *ssd_buf_tag, SSDEvictionStrategy strategy);
static void    *hitInSSDBuffer(SSDBufDesp * ssd_buf_hdr, SSDEvictionStrategy strategy);
static int init_SSDDescriptorBuffer();
static int init_StatisticObj();

/* stopwatch */
static double time_begin_temp;
static double time_now_temp;
static struct timeval   tv_begin_temp, tv_now_temp;
static struct timezone	tz_begin_temp, tz_now_temp;
static void startTimer();
static void stopTimer();
static double lastTimerinterval();

/* Device I/O operation with Timer */
static int dev_pread    (int fd, void* buf,size_t nbytes,off_t offset);
static int dev_pwrite   (int fd, void* buf,size_t nbytes,off_t offset);
static char *ssd_buffer;


/*
 * init buffer hash table, strategy_control, buffer, work_mem
 */
void
initSSD()
{
    int r_initstrategybuf   =   initStrategySSDBuffer(EvictStrategy);
    int r_initbuftb         =   initSSDBufTable(NTABLE_SSD_CACHE);
    int r_initdesp          =   init_SSDDescriptorBuffer();
    int r_initstt           =   init_StatisticObj();
    printf("init_Strategy: %d, init_table: %d, init_desp: %d, inti_Stt: %d\n",r_initstrategybuf, r_initbuftb, r_initdesp, r_initstt);

    int returnCode;
    returnCode = posix_memalign(&ssd_buffer, 512, sizeof(char) * BLCKSZ);
    if (returnCode < 0)
    {
        printf("[ERROR] flushSSDBuffer():--------posix memalign\n");
        free(ssd_buffer);
        exit(-1);
    }
}

static int init_SSDDescriptorBuffer()
{
    int stat = SHM_lock_n_check("LOCK_SSDBUF_DESP");
    if(stat == 0)
    {
        ssd_buf_desp_ctrl = (SSDBufDespCtrl *)SHM_alloc(SHM_SSDBUF_DESP_CTRL,sizeof(SSDBufDespCtrl));
        ssd_buf_desps = (SSDBufDesp *)SHM_alloc(SHM_SSDBUF_DESPS,sizeof(SSDBufDesp) * NBLOCK_SSD_CACHE);

        ssd_buf_desp_ctrl->n_usedssd = 0;
        ssd_buf_desp_ctrl->first_freessd = 0;
        ssd_buf_desp_ctrl->last_freessd = NBLOCK_SSD_CACHE - 1;
        SHM_mutex_init(&ssd_buf_desp_ctrl->lock);

        long i;
        SSDBufDesp  *ssd_buf_hdr = ssd_buf_desps;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr++, i++)
        {
            ssd_buf_hdr->ssd_buf_flag = 0;
            ssd_buf_hdr->ssd_buf_id = i;
            ssd_buf_hdr->next_freessd = i + 1;
        }
        ssd_buf_desps[NBLOCK_SSD_CACHE - 1].next_freessd = -1;

        lock_process_req = (pthread_mutex_t*)SHM_alloc(SHM_PROCESS_REQ_LOCK,sizeof(pthread_mutex_t));
        SHM_mutex_init(lock_process_req);
    }
    else
    {
        ssd_buf_desp_ctrl = (SSDBufDespCtrl *)SHM_get(SHM_SSDBUF_DESP_CTRL,sizeof(SSDBufDespCtrl));
        ssd_buf_desps = (SSDBufDesp *)SHM_get(SHM_SSDBUF_DESPS,sizeof(SSDBufDesp) * NBLOCK_SSD_CACHE);

        lock_process_req = (pthread_mutex_t*)SHM_get(SHM_PROCESS_REQ_LOCK,sizeof(pthread_mutex_t));
    }
    SHM_unlock("LOCK_SSDBUF_DESP");
    return stat;
}

static int init_StatisticObj()
{
    int stat = SHM_lock_n_check("GLOBALSTATISTIC");
    if(stat == 0)
    {
        hit_num = 0;
        flush_ssd_blocks = 0;
        read_ssd_blocks = 0;
        time_read_cmr = 0.0;
        time_write_cmr = 0.0;
        time_read_ssd = 0.0;
        time_write_ssd = 0.0;
        miss_hashitem_num = 0;
	read_hashmiss = 0;
	write_hashmiss = 0;
	read_hit_num = 0;
    }
    SHM_unlock("GLOBALSTATISTIC");
    return stat;
}

void           *
flushSSDBuffer(SSDBufDesp * ssd_buf_hdr)
{
    int		returnCode;
    if (BandOrBlock == 1)
    {
        SSD_BUFFER_SIZE = BNDSZ;
        BLCKSZ = BNDSZ;
    }


    dev_pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
    time_read_ssd += lastTimerinterval();
    read_ssd_blocks++;

    dev_pwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_tag.offset);
    time_write_smr += lastTimerinterval();
    flush_fifo_blocks++;
    return NULL;
}

static SSDBufDesp *
SSDBufferAlloc(SSDBufferTag *ssd_buf_tag, bool * found, int alloc4What)
{
    SSDBufDesp      *ssd_buf_hdr;
    unsigned long   ssd_buf_hash = ssdbuftableHashcode(ssd_buf_tag);
    long            ssd_buf_id = ssdbuftableLookup(ssd_buf_tag, ssd_buf_hash);

    /** lock **/
    if (ssd_buf_id >= 0)    // when cache hit
    {
        ssd_buf_hdr = &ssd_buf_desps[ssd_buf_id];
        if(isSamebuf(&ssd_buf_hdr->ssd_buf_tag,ssd_buf_tag))
        {
            hitInSSDBuffer(ssd_buf_hdr, EvictStrategy); //need lock
            hit_num++;
            *found = 1;
            return ssd_buf_hdr;
        }
        else
        {
            /** passive delete hash item, which corresponding cache buf has been evicted early **/
            ssdbuftableDelete(ssd_buf_tag,ssd_buf_hash);
            miss_hashitem_num++;
	    if(alloc4What == 1)	// alloc for write
	        write_hashmiss++;
	    else		//alloc for read
		read_hashmiss++;
        }
    }

    /* when cache miss */
    *found = 0;

    ssd_buf_hdr = getSSDStrategyBuffer(ssd_buf_tag, EvictStrategy); //need look
    ssdbuftableInsert(ssd_buf_tag, ssd_buf_hash, ssd_buf_hdr->ssd_buf_id);
    ssd_buf_hdr->ssd_buf_flag &= ~(SSD_BUF_VALID | SSD_BUF_DIRTY);
    CopySSDBufTag(&ssd_buf_hdr->ssd_buf_tag,ssd_buf_tag);

    return ssd_buf_hdr;
}

static int
initStrategySSDBuffer(SSDEvictionStrategy strategy)
{
    /** Add for multi-user **/
    if (strategy == LRU_global)
        return initSSDBufferForLRU();
    else if (strategy == CLOCK)
        initSSDBufferForClock();
    else if (strategy == LRU)
        return initSSDBufferForLRU();
    else if (strategy == LRUofBand)
        initSSDBufferForLRUofBand();
    else if (strategy == Most || strategy == Most_Dirty)
        initSSDBufferForMost();
    else if (strategy == SCAN)
        initSSDBufferForSCAN();
    else if (strategy == WA)
        initSSDBufferForWA();
    else if (strategy == MaxCold)
        initSSDBufferForMaxCold();
    else if (strategy == MaxAll)
        initSSDBufferForMaxCold();
    else if (strategy == AvgBandHot)
        initSSDBufferForMaxCold();
    else if (strategy == HotDivSize)
        initSSDBufferForMaxCold();
    return 0;
}

static SSDBufDesp *
getSSDStrategyBuffer(SSDBufferTag *ssd_buf_tag, SSDEvictionStrategy strategy)
{
    if (strategy == CLOCK)
        return getCLOCKBuffer();
    else if (strategy == LRU)
        return getLRUBuffer();
    else if (strategy == LRUofBand)
        return getLRUofBandBuffer(ssd_buf_tag);
    else if (strategy == Most)
        return getMostBuffer(ssd_buf_tag);
    else if (strategy == SCAN)
        return getSCANBuffer(ssd_buf_tag);
    else if (strategy == WA)
        return getWABuffer(ssd_buf_tag);
    else if (strategy == MaxCold)
        return getMaxColdBuffer(ssd_buf_tag, strategy);
    else if (strategy == MaxAll)
        return getMaxColdBuffer(ssd_buf_tag, strategy);
    else if (strategy == AvgBandHot)
        return getMaxColdBuffer(ssd_buf_tag, strategy);
    else if (strategy == HotDivSize)
        return getMaxColdBuffer(ssd_buf_tag, strategy);
}

static void    *
hitInSSDBuffer(SSDBufDesp * ssd_buf_hdr, SSDEvictionStrategy strategy)
{
    if (strategy == CLOCK)
        hitInCLOCKBuffer(ssd_buf_hdr);
    else if (strategy == LRU)
        hitInLRUBuffer(ssd_buf_hdr);
    else if (strategy == LRUofBand)
        hitInLRUofBandBuffer(ssd_buf_hdr);
    else if (strategy == Most || strategy == Most_Dirty)
        hitInMostBuffer();
    else if (strategy == SCAN)
        hitInSCANBuffer(ssd_buf_hdr);
    else if (strategy == WA)
        hitInWABuffer(ssd_buf_hdr);
    else if (strategy == MaxCold)
        hitInMaxColdBuffer(ssd_buf_hdr);
    else if (strategy == MaxAll)
        hitInMaxColdBuffer(ssd_buf_hdr);
    else if (strategy == AvgBandHot)
        hitInMaxColdBuffer(ssd_buf_hdr);
    else if (strategy == HotDivSize)
        hitInMaxColdBuffer(ssd_buf_hdr);
}

/*
 * read--return the buf_id of buffer according to buf_tag
 */

void
read_block(off_t offset, char *ssd_buffer)
{
    bool found = 0;
    static SSDBufferTag ssd_buf_tag;
    static SSDBufDesp *ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    if (DEBUG)
        printf("[INFO] read():-------offset=%lu\n", offset);
//    if (EvictStrategy == CMR)
//    {
//        read_smr_blocks++;
//        gettimeofday(&tv_begin_temp, &tz_begin_temp);
//        time_begin_temp = tv_begin_temp.tv_sec + tv_begin_temp.tv_usec / 1000000.0;
//        returnCode = pread(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] read_block():-------read from cmr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
//            exit(-1);
//        }
//        gettimeofday(&tv_now_temp, &tz_now_temp);
//        time_now_temp = tv_now_temp.tv_sec + tv_now_temp.tv_usec / 1000000.0;
//        time_read_cmr += time_now_temp - time_begin_temp;
//    }
//    else if (EvictStrategy == SMR)
//    {
//        returnCode = pread(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_tag.offset);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] read_block():---------SMR\n");
//            exit(-1);
//        }
//    }
//    else //LRU ...
//    {
    ssd_buf_hdr = SSDBufferAlloc(&ssd_buf_tag, &found, 0);
    if (found)
    {
        read_hit_num++;
        dev_pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        time_read_ssd += lastTimerinterval();
        read_ssd_blocks++;
    }
    else
    {
        dev_pread(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
        time_read_smr += lastTimerinterval();
        read_smr_blocks++;

        dev_pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        time_write_ssd += lastTimerinterval();
        flush_ssd_blocks++;
    }
    ssd_buf_hdr->ssd_buf_flag &= ~SSD_BUF_DIRTY;
    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID;
//    }
}

/*
 * write--return the buf_id of buffer according to buf_tag
 */
void
write_block(off_t offset,int blkcnt, char *ssd_buffer)
{
    bool	found;

    static SSDBufferTag ssd_buf_tag;
    static SSDBufDesp   *ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    if (DEBUG)
        printf("[INFO] write():-------offset=%lu\n", offset);
//    if (EvictStrategy == CMR)
//    {
//        flush_bands++;
//        returnCode = pwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():-------write to smr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
//            exit(-1);
//        }
//        //returnCode = fsync(smr_fd);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():----------fsync\n");
//            exit(-1);
//        }
//
//    }
//    else if (EvictStrategy == SMR)
//    {
//        returnCode = pwrite(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_tag.offset);
//        if (returnCode < 0)
//        {
//            printf("[ERROR] write_block():---------SMR\n");
//            exit(-1);
//        }
//    }
//    else    //LRU ...
//    {

    ssd_buf_hdr = SSDBufferAlloc(&ssd_buf_tag, &found, 1);



    dev_pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
    time_write_ssd += lastTimerinterval();
    flush_ssd_blocks++;

    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID | SSD_BUF_DIRTY;
//    }
}

void
read_band(off_t offset, char *ssd_buffer)
{
    if (BandOrBlock == 1)
    {
        SSD_BUFFER_SIZE = BNDSZ;
    }
    printf("enter read_band\n");
    void           *ssd_buf_block;
    bool		found = 0;
    int		returnCode;

    static SSDBufferTag ssd_buf_tag;
    static SSDBufDesp *ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    static SSDBufferTag band_tag;
    band_tag.offset = (offset / BNDSZ);
    static SSDBufferTag hdr_tag;
    hdr_tag.offset = (band_tag.offset) * BNDSZ;
    size_t		new_offset = offset - hdr_tag.offset;
    char           *band_buffer;
    returnCode = posix_memalign(&band_buffer, 512, sizeof(char) * BNDSZ);
    if (returnCode < 0)
    {
        printf("[ERROR] read_band():-------posix_memalign\n");
        exit(-1);
    }
    printf("readband_tag%ld\n", band_tag.offset);
    if (DEBUG)
        printf("[INFO] read():-------offset=%lu\n", offset);
    ssd_buf_hdr = SSDBufferAlloc(&hdr_tag, &found, 0);
    if (found)
    {
        returnCode = pread(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE + new_offset);
        if (returnCode < 0)
        {
            printf("[ERROR] read():-------read from ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
            exit(-1);
        }
    }
    else
    {
        //returnCode = smrread(hdd_fd, band_buffer, BNDSZ, hdr_tag.offset);
        returnCode = pread(hdd_fd, ssd_buffer, SSD_BUFFER_SIZE, offset);
        if (returnCode < 0)
        {
            printf("[ERROR] read():-------read from smr: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
            exit(-1);
        }
        flush_ssd_blocks++;
        returnCode = pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * SSD_BUFFER_SIZE);
        returnCode = pwrite(ssd_fd, band_buffer, BNDSZ, ssd_buf_hdr->ssd_buf_id * BNDSZ);
        if (returnCode < 0)
        {
            printf("[ERROR] read():-------write to ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
            exit(-1);
        }
    }
    ssd_buf_hdr->ssd_buf_flag &= ~SSD_BUF_DIRTY;
    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID;
}
void
write_band(off_t offset, char *ssd_buffer)
{
    if (BandOrBlock == 1)
    {
        SSD_BUFFER_SIZE = BNDSZ;
    }
    void           *ssd_buf_block;
    bool		found;
    int		returnCode;

    static SSDBufferTag ssd_buf_tag;
    static SSDBufDesp *ssd_buf_hdr;

    ssd_buf_tag.offset = offset;
    static SSDBufferTag band_tag;
    band_tag.offset = (offset / BNDSZ);
    static SSDBufferTag hdr_tag;
    hdr_tag.offset = (band_tag.offset) * BNDSZ;
    size_t		new_offset = offset - hdr_tag.offset;
    char           *band_buffer;
    if (DEBUG)
        printf("[INFO] write():-------offset=%lu\n", offset);

    returnCode = posix_memalign(&band_buffer, 512, sizeof(char) * BNDSZ);
    if (returnCode < 0)
    {
        printf("[ERROR] write_band():-------posix_memalign\n");
        exit(-1);
    }
    ssd_buf_hdr = SSDBufferAlloc(&hdr_tag, &found, 1);
    flush_ssd_blocks++;

    if (found)
    {
        returnCode = pwrite(ssd_fd, ssd_buffer, SSD_BUFFER_SIZE, ssd_buf_hdr->ssd_buf_id * BNDSZ + new_offset);
    }
    else
    {
        returnCode = pread(hdd_fd, band_buffer, BNDSZ, hdr_tag.offset);

        if (returnCode < 0)
        {
            printf("[ERROR] write():-------write to ssd: fd=%d, errorcode=%d, offset=%lu\n", ssd_fd, returnCode, offset);
            exit(-1);
        }
        memcpy(band_buffer + new_offset, ssd_buffer, BLCKSZ);
    }
    ssd_buf_hdr->ssd_buf_flag |= SSD_BUF_VALID | SSD_BUF_DIRTY;

}


/******************
**** Utilities*****
*******************/

static int dev_pread(int fd, void* buf,size_t nbytes,off_t offset)
{
        startTimer();
        int r = pread(fd,buf,nbytes,offset);
        stopTimer();
        if (r < 0)
        {
            printf("[ERROR] read():-------read from device: fd=%d, errorcode=%d, offset=%lu\n", fd, r, offset);
            exit(-1);
        }
        return r;
}

static int dev_pwrite(int fd, void* buf,size_t nbytes,off_t offset)
{
        startTimer();
        int w = pwrite(fd,buf,nbytes,offset);
        stopTimer();
        if (w < 0)
        {
            printf("[ERROR] read():-------read from device: fd=%d, errorcode=%d, offset=%lu\n", fd, w, offset);
            exit(-1);
        }
        return w;
}

static void startTimer()
{
        gettimeofday(&tv_begin_temp, &tz_begin_temp);
        time_begin_temp = tv_begin_temp.tv_sec + tv_begin_temp.tv_usec / 1000000.0;
}

static void stopTimer()
{
        gettimeofday(&tv_begin_temp, &tz_begin_temp);
        time_now_temp = tv_begin_temp.tv_sec + tv_begin_temp.tv_usec / 1000000.0;
}

static double lastTimerinterval()
{
    return time_now_temp - time_begin_temp;
}

void CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag)
{
    objectTag->offset = sourceTag->offset;
}


bool isSamebuf(SSDBufferTag *tag1, SSDBufferTag *tag2)
{
	if (tag1->offset != tag2->offset)
		return 0;
	else return 1;
}
