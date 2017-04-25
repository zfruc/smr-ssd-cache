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

void
trace_to_iocall(char *trace_file_path)
{
    char		action;
    off_t		offset;
    size_t		size;
    char       *ssd_buffer;
    float		size_float;
    int	        returnCode;

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

    while (!feof(trace))
    {
        returnCode = fscanf(trace, "%c %d %lu\n", &action, &i, &offset);
        if (returnCode < 0)
        {
            error("error while reading trace file.");
            break;
        }
        size = 4096;
        offset = offset * BLCKSZ;

//		unsigned long	offset_end = offset + size;
//		if (offset % 4096 != 0)
//			offset = offset / 4096 * 4096;
//		if (offset_end % 4096 != 0)
//			size = offset_end / 4096 * 4096 - offset + 4096;
//		else
//			size = offset_end - offset;


        if (action == ACT_WRITE)
        {
            if (DEBUG)
                printf("[INFO] trace_to_iocall():wirte offset=%lu\n", offset);
            write_block(offset,1, ssd_buffer);
        }
        else if (action == ACT_READ)
        {
            if (DEBUG)
                printf("[INFO] trace_to_iocall():read offset=%lu\n", offset);
            read_block(offset,ssd_buffer);
        }
    }
    reportCurInfo();
    free(ssd_buffer);
    fclose(trace);
}

static void reportCurInfo()
{
        printf(" read_hit_num:%lu\n hit num:%lu\n read_ssd_blocks:%lu\n flush_ssd_blocks:%lu\n read_fifo_blocks:%lu\n flush_fifo_blocks:%lu\n read_smr_blocks:%lu\n read_smr_bands:%lu\n flush_bands:%lu\n flush_band_size=%lu\n",
                 read_hit_num, hit_num, read_ssd_blocks, flush_ssd_blocks, read_fifo_blocks, flush_fifo_blocks, read_smr_blocks, read_smr_bands, flush_bands, flush_band_size);

}
