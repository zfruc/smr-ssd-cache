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

char READ = '0';
char WRITE = '1';
multiuser_trace_call()
{

}
void
trace_to_iocall(char *trace_file_path)
{
    FILE *trace;
    if ((trace = fopen(trace_file_path, "rt")) == NULL)
    {
        printf("[ERROR] trace_to_iocall():--------Fail to open the trace file!");
        exit(1);
    }

    char		action;
    char		write_or_read[100];
    off_t		offset;
    size_t		size;
    char           *ssd_buffer;
    int		i;
    float		size_float;

    int	returnCode = posix_memalign(&ssd_buffer, 512, 16*sizeof(char) * BLCKSZ);
    if (returnCode < 0)
    {
        printf("[ERROR] flushSSDBuffer():--------posix memalign\n");
        free(ssd_buffer);
        exit(-1);
    }

    for (i = 0; i < 16*BLCKSZ; i++)
        ssd_buffer[i] = '1';
    while (!feof(trace))
    {
//        returnCode = fscanf(trace, "%c %d %lu\n", &action, &i, &offset);
//        if (returnCode < 0)
//            break;

        size = 4096;
        offset = offset * BLCKSZ;

//		unsigned long	offset_end = offset + size;
//		if (offset % 4096 != 0)
//			offset = offset / 4096 * 4096;
//		if (offset_end % 4096 != 0)
//			size = offset_end / 4096 * 4096 - offset + 4096;
//		else
//			size = offset_end - offset;


        if (action == WRITE)
        {
            if (DEBUG)
                printf("[INFO] trace_to_iocall():--------wirte offset=%lu\n", offset);
            write_block(offset,1, ssd_buffer);
        }
        else if (action == READ)
        {
            if (DEBUG)
                printf("[INFO] trace_to_iocall():--------read offset=%lu\n", offset);
        }
            }
    printf("read_hit_num:%lu  hit num:%lu   read_ssd_blocks:%lu  flush_ssd_blocks:%lu flush_times:%lu read_fifo_blocks:%lu   flush_fifo_blocks:%lu  read_smr_blocks:%lu   read_smr_bands:%lu   flush_bands:%lu flush_band_size=%lu\n ", read_hit_num, hit_num, read_ssd_blocks, flush_ssd_blocks, flush_times, read_fifo_blocks, flush_fifo_blocks, read_smr_blocks, read_smr_bands, flush_bands, flush_band_size);
    free(ssd_buffer);
    fclose(trace);
}
