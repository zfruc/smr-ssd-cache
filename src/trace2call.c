#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "strategy/clock.h"
#include "strategy/lru.h"
#include "strategy/lruofband.h"
#include "strategy/scan.h"
#include "trace2call.h"
#include "report.h"
void            trace_to_iocall(char *trace_file_path);
static void     reportCurInfo();
static void report_ontime();
static void resetStatics();

static double time_begin, time_now;
static struct timeval  tv_begin, tv_now;
static struct timezone tz_begin, tz_now;
unsigned long totalreq_cnt = 0;

void
trace_to_iocall(char *trace_file_path)
{
    char		action;
    off_t		offset;
    char       *ssd_buffer;
    float		size_float;
    int	        returnCode;
    int         isFullSSDcache = 0;
    FILE *trace;
    if ((trace = fopen(trace_file_path, "rt")) == NULL)
    {
        error("Failed to open the trace file!\n");
        exit(1);
    }

    returnCode = posix_memalign(&ssd_buffer, 512, 16*sizeof(char) * BLCKSZ);
    if (returnCode < 0)
    {
        error("posix memalign error\n");
        free(ssd_buffer);
        exit(-1);
    }
    int i;
    for (i = 0; i < 16*BLCKSZ; i++)
    {
        ssd_buffer[i] = '1';
    }

    gettimeofday(&tv_begin, &tz_begin);
    time_begin = tv_begin.tv_sec + tv_begin.tv_usec / 1000000.0;
    while (!feof(trace))
    {
        returnCode = fscanf(trace, "%c %d %lu\n", &action, &i, &offset);
        if (returnCode < 0)
        {
            error("error while reading trace file.");
            break;
        }
        offset = offset * BLCKSZ;

        if(!isFullSSDcache && flush_fifo_blocks > 0)
        {
            reportCurInfo();
            resetStatics();        // Because we do not care about the statistic while the process of filling SSD cache.
            isFullSSDcache = 1;
        }

        SHM_mutex_lock(lock_process_req);
        totalreq_cnt++;
        if (action == ACT_WRITE)
        {
            if (DEBUG)
                printf("[INFO] trace_to_iocall():wirte offset=%lu\n", offset);
            if(totalreq_cnt==55)
                totalreq_cnt =55;
            write_block(offset,1, ssd_buffer);
        }
        else if (action == ACT_READ)
        {
            if (DEBUG)
                printf("[INFO] trace_to_iocall():read offset=%lu\n", offset);
            read_block(offset,ssd_buffer);
        }
        SHM_mutex_unlock(lock_process_req);

        if (totalreq_cnt % 10000 == 0)
            report_ontime();

    }
    reportCurInfo();
    free(ssd_buffer);
    fclose(trace);
}

static void reportCurInfo()
{
    gettimeofday(&tv_now, &tz_now);
    time_now = tv_now.tv_sec + tv_now.tv_usec / 1000000.0;
    printf(" totalreqNum:%u\n read_hit_num:%lu\n hit num:%lu\n read_ssd_blocks:%lu\n flush_ssd_blocks:%lu\n flush_fifo_blocks:%lu\n read_smr_blocks:%lu\n hash_miss:%lu\n",
           totalreq_cnt, read_hit_num, hit_num, read_ssd_blocks, flush_ssd_blocks, flush_fifo_blocks, read_smr_blocks, miss_hashitem_num);
    printf(" total run time (s) = %lf\ntime_read_ssd = %lf\n time_write_ssd = %lf\ntime_read_smr = %lf\ntime_write_smr = %lf\n",
           time_now - time_begin, time_read_ssd, time_write_ssd, time_read_smr, time_write_smr);
}

static void resetStatics()
{
	hit_num = 0;
	flush_bands = 0;
	flush_band_size = 0;
	flush_fifo_blocks = 0;
	flush_ssd_blocks = 0;
	read_ssd_blocks = 0;
	read_fifo_blocks = 0;
	read_smr_blocks = 0;
	read_hit_num = 0;
	read_smr_bands = 0;
	time_read_cmr = 0;
	time_write_cmr = 0;
	time_read_ssd = 0;
	time_write_ssd = 0;
	time_read_fifo = 0;
	time_read_smr = 0;
	time_write_fifo = 0;
	time_write_smr = 0;
	totalreq_cnt = 0;
}

static void report_ontime()
{
    printf("totalreqNum:%u, hit num:%lu   flush_ssd_blocks:%lu flush_fifo_blocks:%lu, hashmiss:%lu\n",totalreq_cnt, hit_num, flush_ssd_blocks, flush_fifo_blocks, miss_hashitem_num);
}
