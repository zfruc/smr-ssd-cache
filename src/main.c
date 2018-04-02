/*
 * main.c
 */
#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>

#include "report.h"
#include "shmlib.h"
#include "global.h"
#include "cache.h"
#include "smr-simulator/smr-simulator.h"
#include "smr-simulator/simulator_logfifo.h"
#include "smr-simulator/simulator_v2.h"
#include "trace2call.h"
#include "daemon.h"
#include "timerUtils.h"
//static char str_program_config[];

unsigned int INIT_PROCESS = 0;
void ramdisk_iotest()
{
    int fdram = open("/mnt/ramdisk/ramdisk",O_RDWR | O_DIRECT);
    printf("fdram=%d\n",fdram);

    char* buf;
    posix_memalign(&buf, 512, 4096);
    size_t count = 512;
    off_t offset = 0;

    int r;
    while(1)
    {
        r = pwrite(fdram,buf,count,offset);
        if(r<=0)
        {
            printf("write ramdisk error:%d\n",r);
            exit(1);
        }
    }
}

char* tracefile[] = {"/home/trace/src1_2.csv.req",
                     "/home/trace/wdev_0.csv.req",
                     "/home/trace/hm_0.csv.req",
                     "/home/trace/mds_0.csv.req",
                     "/home/trace/prn_0.csv.req",       //1 1 4 0 0 106230 5242880 0
                     "/home/trace/rsrch_0.csv.req",
                     "/home/trace/stg_0.csv.req",
                     "/home/trace/ts_0.csv.req",
                     "/home/trace/usr_0.csv.req",
                     "/home/trace/web_0.csv.req",
                     "/home/trace/production-LiveMap-Backend-4K.req", // --> not in used.
                     "/home/trace/merged_traceX18.req"  // default set: cache size = 8M*blksize; persistent buffer size = 1.6M*blksize.
                    //"/home/trace/merged_trace_x1.req.csv"
                    };

blksize_t trace_req_total[] = {14024860,2654824,8985487,2916662,17635766,3254278,6098667,4216457,12873274,9642398,1,1481448114};

int
main(int argc, char** argv)
{

    //FunctionalTest();
    //ramdisk_iotest();

// 1 1 1 0 0 100000 100000
// 1 1 0 0 0 100000 100000

// trace11: 0 0 11 1 0 8000000 35 PV3
    if (argc == 10)
    {
        BatchId = atoi(argv[1]);
        UserId = atoi(argv[2]);
        TraceId = atoi(argv[3]);
        WriteOnly = atoi(argv[4]);
        StartLBA = atol(argv[5]);
        NBLOCK_SSD_CACHE = NTABLE_SSD_CACHE = atol(argv[6]);
        NBLOCK_SMR_FIFO = atol(argv[7]) * (ZONESZ / BLCKSZ);

        if (strcmp(argv[8],"LRU") == 0)
            EvictStrategy = LRU_private;
        else if (strcmp(argv[8],"LRU_RW") == 0)
            EvictStrategy = LRU_rw;
        else if (strcmp(argv[8],"PV3") == 0)
            EvictStrategy = PV3;
        else if (strcmp(argv[8],"PORE") == 0)
            EvictStrategy = PORE;
	else if(strcmp(argv[8],"MOST") == 0)
	    EvictStrategy = MOST;
	if(atoi(argv[9]) < 0)
	    PeriodLenth = NBLOCK_SMR_FIFO;
	else
    	    PeriodLenth =  atoi(argv[9]) * (ZONESZ / BLCKSZ);
    	#ifdef CACHE_PROPORTIOIN_STATIC
    	Proportion_Dirty = atof(argv[10]);
    	#endif // Proportion_Dirty

        //EvictStrategy = PORE_PLUS;
    }
    else
    {
        printf("parameters are wrong %d\n", argc);
        exit(-1);
    }


#ifdef CG_THROTTLE
    init_cgdev();
#endif // CG_THROTTLE

    initLog();
    initRuntimeInfo();
    STT->trace_req_amount = trace_req_total[TraceId];
    initSSD();

    ssd_fd = open(ssd_device, O_RDWR | O_DIRECT);
#ifdef SIMULATION
    fd_fifo_part = open(simu_smr_fifo_device, O_RDWR | O_DIRECT);
    fd_smr_part = open(simu_smr_smr_device, O_RDWR | O_DIRECT | O_FSYNC);
    printf("Simulator Device: fifo part=%d, smr part=%d\n",fd_fifo_part,fd_smr_part);
    if(fd_fifo_part<0 || fd_smr_part<0) exit(-1);
    InitSimulator();
#else
    hdd_fd = open(smr_device, O_RDWR | O_DIRECT);
    printf("Device ID: hdd=%d, ssd=%d\n",hdd_fd,ssd_fd);

#endif
#ifdef DAEMON_PROC
    pthread_t tid;
    int err = pthread_create(&tid, NULL, daemon_proc, NULL);
    if (err != 0)
    {
        printf("[ERROR] initSSD: fail to create thread: %s\n", strerror(err));
        exit(-1);
    }
#endif // DAEMON
    trace_to_iocall(tracefile[TraceId],WriteOnly,StartLBA);

#ifdef SIMULATION
    PrintSimulatorStatistic();
#endif
    close(hdd_fd);
    close(ssd_fd);
    CloseLogFile();
	ReportCM();
    return 0;
}

int initRuntimeInfo()
{
    char str_STT[50];
    sprintf(str_STT,"STAT_b%d_u%d_t%d",BatchId,UserId,TraceId);
    STT = (struct RuntimeSTAT*)multi_SHM_alloc(str_STT,sizeof(struct RuntimeSTAT));
    if(STT == NULL)
        return errno;

    STT->batchId = BatchId;
    STT->userId = UserId;
    STT->traceId = TraceId;
    STT->startLBA = StartLBA;
    STT->isWriteOnly = WriteOnly;
    STT->cacheUsage = 0;
    STT->cacheLimit = 0x7fffffffffffffff;

    STT->wtrAmp_cur = 0;
    STT->WA_sum = 0;
    STT->n_RMW = 0;
    return 0;
}

int init_cgdev()
{
    sprintf(ram_device,"/mnt/ramdisk/ramdisk_%d",STT->userId);
    ram_fd = open(ram_device,O_RDWR | O_DIRECT);
    printf("fdram=%d\n",ram_fd);
    return ram_fd;
}


int initLog()
{
    char logpath[50];
    sprintf(logpath,"%s/20180317-RWcost.log",PATH_LOG,TraceId);
    int rt = 0;
    if((rt = OpenLogFile(logpath)) < 0)
    {
        error("open log file failure.\n");
        exit(1);
    }
    return rt;
}
