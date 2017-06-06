#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "timerUtils.h"
#include "ssd-cache.h"
#include "strategy/lru.h"
#include "trace2call.h"
#include "report.h"

static void reportCurInfo();
static void report_ontime();
static void resetStatics();

static timeval   tv_trace_start, tv_trace_end;
static double time_trace;

/** single request statistic information **/
static timeval   tv_req_start, tv_req_stop;
static microsecond_t  msec_req;
extern microsecond_t  msec_r_hdd,msec_w_hdd,msec_r_ssd,msec_w_ssd;
extern int IsHit;
char logbuf[512];

unsigned long totalreq_cnt = 0;
unsigned long readreq_cnt = 0;
unsigned long writereq_cnt = 0;


void
trace_to_iocall(char *trace_file_path, int isWriteOnly,off_t startLBA)
{
    char		action;
    off_t		offset;
    char       *ssd_buffer;
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
    for (i = 0; i < 16 * BLCKSZ; i++)
    {
        ssd_buffer[i] = '1';
    }

    TimerStart(&tv_trace_start);
    while (!feof(trace))
    {
        returnCode = fscanf(trace, "%c %d %lu\n", &action, &i, &offset);
        if (returnCode < 0)
        {
            error("error while reading trace file.");
            break;
        }
        offset = (offset + startLBA) * BLCKSZ;

        if(!isFullSSDcache && flush_clean_blocks > 0)
        {
            reportCurInfo();
            resetStatics();        // Because we do not care about the statistic while the process of filling SSD cache.
            isFullSSDcache = 1;
        }

        TimerStart(&tv_req_start);
        if (action == ACT_WRITE) // Write = 1
        {
            totalreq_cnt++;
            writereq_cnt++;
            if (DEBUG)
                printf("[INFO] trace_to_iocall():wirte offset=%lu\n", offset);

            write_block(offset, ssd_buffer);
        }
        else if (!isWriteOnly && action == ACT_READ)    // read = 9
        {
            totalreq_cnt++;
            readreq_cnt++;
            if (DEBUG)
                printf("[INFO] trace_to_iocall():read offset=%lu\n", offset);
            read_block(offset,ssd_buffer);
        }

        if (totalreq_cnt % 10000 == 0)
            report_ontime();

        TimerStop(&tv_req_stop);
        msec_req = GetTimerInterval(&tv_req_start,&tv_req_stop);
        /*
            print log
            format:
            <req_id, r/w, ishit, time cost for: one request, read_ssd, write_ssd, read_smr, write_smr>
        */
        sprintf(logbuf,"%lu,%c,%d,%ld,%ld,%ld,%ld,%ld\n",totalreq_cnt,action,IsHit,msec_req,msec_r_ssd,msec_w_ssd,msec_r_hdd,msec_w_hdd);
        WriteLog(logbuf);
        msec_r_ssd = msec_w_ssd = msec_r_hdd = msec_w_hdd = 0;
    }
    TimerStop(&tv_trace_end);
    time_trace = Mirco2Sec(GetTimerInterval(&tv_trace_start,&tv_trace_end));
    reportCurInfo();
    free(ssd_buffer);
    fclose(trace);
}

static void reportCurInfo()
{
    printf(" totalreqNum:%lu\n read_req_count: %lu\n write_req_count: %lu\n",
            totalreq_cnt,readreq_cnt,writereq_cnt);

    printf(" hit num:%lu\n read_hit_num:%lu\n write_hit_num:%lu\n",
            hit_num,read_hit_num,write_hit_num);

    printf(" read_ssd_blocks:%lu\n flush_ssd_blocks:%lu\n read_hdd_blocks:%lu\n flush_hdd_blocks:%lu\n flush_clean_blocks:%lu\n",
            load_ssd_blocks, flush_ssd_blocks, load_hdd_blocks, flush_hdd_blocks, flush_clean_blocks);

    printf(" hash_miss:%lu\n hashmiss_read:%lu\n hashmiss_write:%lu\n",
            hashmiss_sum, hashmiss_read, hashmiss_write);

    printf(" total run time (s) : %lf\n time_read_ssd : %lf\n time_write_ssd : %lf\n time_read_smr : %lf\n time_write_smr : %lf\n",
            time_trace, time_read_ssd, time_write_ssd, time_read_hdd, time_write_hdd);
}

static void resetStatics()
{
    hit_num = 0;
    load_ssd_blocks = 0;
    load_hdd_blocks = 0;
    flush_ssd_blocks = 0;
    flush_hdd_blocks = 0;
    flush_clean_blocks = 0;
    read_hit_num = 0;
    write_hit_num = 0;
    time_read_hdd = 0;
    time_write_hdd = 0;
    time_read_ssd = 0;
    time_write_ssd = 0;
    totalreq_cnt = 0;
    readreq_cnt = 0;
    hashmiss_sum = 0;
    hashmiss_read = 0;
    hashmiss_write = 0;
}

static void report_ontime()
{
    printf("totalreqNum:%lu, readreqNum:%lu, hit num:%lu, readhit:%lu, flush_ssd_blocks:%lu flush_hdd_blocks:%lu, hashmiss:%lu, readhassmiss:%lu writehassmiss:%lu\n",
           totalreq_cnt,readreq_cnt, hit_num, read_hit_num, flush_ssd_blocks, flush_hdd_blocks, hashmiss_sum, hashmiss_read, hashmiss_write);}
