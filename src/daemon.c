#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "global.h"
#include "daemon.h"

extern struct RuntimeSTAT* STT;

void* daemon_proc()
{
    char path[] = "/tmp/daemon_cachesys.out";
    unlink(path);
    FILE* file = fopen(path, "at+"); // append mode
    char str_runtime[4096];

    pthread_t th = pthread_self();
    pthread_detach(th);

    time_t rawtime;
    struct tm * timeinfo;

    /* Bandwidth */
    int bw_s, bw_r, bw_w; // KB/s
    blkcnt_t lastcnt_s,lastcnt_r,lastcnt_w;
    lastcnt_s = lastcnt_r = lastcnt_w = 0;

    while(1)
    {
        sleep(1);
    /* Load runtime arverage IO speed(KB/s) */
        blksize_t reqcnt_s = STT->reqcnt_s;
        blksize_t reqcnt_r = STT->reqcnt_r;
        blksize_t reqcnt_w = STT->reqcnt_w;

        bw_s = (reqcnt_s - lastcnt_s) * 4;
	bw_r = (reqcnt_r - lastcnt_r) * 4;
	bw_w = (reqcnt_w - lastcnt_w) * 4;
        lastcnt_s = reqcnt_s;
	lastcnt_r = reqcnt_r;
	lastcnt_w = reqcnt_w;

    /* Current WrtAmp */
        double wrtamp = STT->wtrAmp_cur;
    /* Process Percentage */
        double progress = (double)STT->reqcnt_s / STT->trace_req_amount * 100;

    /* Output */
        char timebuf [128];
        timeinfo = localtime (&rawtime);
        strftime (timebuf,sizeof(timebuf),"%Y/%m/%d %H:%M:%S",timeinfo);

	sprintf(str_runtime, "[%s]bandwidth(s,r,w):%d,\t%d,\t%d\nreqcnt(s,r,w):%ld\t%ld\t%ld\n",timebuf,bw_s,bw_r,bw_w,reqcnt_s,reqcnt_r,reqcnt_w);
        fwrite(str_runtime,strlen(str_runtime),1,file);
	sprintf(str_runtime, "wrtAmp:%d\tprogress:%d\%\n",(int)wrtamp,(int)progress);
        fwrite(str_runtime,strlen(str_runtime),1,file);
	fflush(file);
    }
}

