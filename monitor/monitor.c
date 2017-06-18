#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>             /* getopt */
#include "global.h"
#include "shmlib.h"

const char* PATH_SHM = "/dev/shm";

static char cmdbuf[1024];
static void ReadCommond();
static int readANS(char* buf,int cnt);

char* str_help = "Command action\n\ta\tCheck processes runtime state.\n\tp\tRe-Size the Cache Usage upper limitation.\n";

int
main(int argc, char** argv)
{
    while(1)
    {
        ReadCommond();
    }
}

void CheckRuntime()
{
    struct RuntimeSTAT* usrStat;
    char* str_selectbatch = "Batch ID: ";
    char* str_selectuser = "User ID: ";
    char* str_nouser = "There is no the user\n";
    char* str_pipeerr = "something error...\n";

    char str_buf[1024];

    char str_bid[10];
    char str_uid[10];
    int batchId;
    int usrId;
    FILE* fd_shell;
    char command[1024];
    char shellbuf[1024];

    while(1)
    {
        Ask(str_selectbatch);
        if(readANS(str_bid,9)<0) return;
        batchId = atoi(str_bid);
        sprintf(command,"ls -l %s/STAT_b%d_* | grep \"^-\"|wc -l",PATH_SHM,batchId);

        fd_shell = popen(command,"r");
        if(fd_shell <=0 )
        {
            Say(str_pipeerr);
            continue;
        }
        fgets(shellbuf,1023,fd_shell);
        pclose(fd_shell);

        int usercnt = atoi(shellbuf);
        sprintf(str_buf,"%d Users in this batch.\n",usercnt);
        Say(str_buf);
        if(usercnt==0) continue;

        do
        {
            Ask(str_selectuser);
            if(readANS(str_uid,9)<0) return;
        }
        while(str_uid[0] == '\n');
        usrId = atoi(str_uid);
        sprintf(command,"ls %s/STAT_b%d_u%d*",PATH_SHM,batchId,usrId);

        fd_shell = popen(command,"r");
        if(fd_shell <= 0)
        {
            Say(str_pipeerr);
            continue;
        }
        fgets(shellbuf,1023,fd_shell);

        int len = strlen(shellbuf);
        if(len <= 0)
        {
            Say(str_nouser);
            continue;
        }
        shellbuf[len-1]=0;
        char* fname = strrchr(shellbuf,'/');
        fname++;

        usrStat = (struct RuntimeSTAT*)SHM_get(fname,sizeof(struct RuntimeSTAT));
        if(usrStat == NULL)
        {
            Say(str_pipeerr);
            continue;
        }

        sprintf(str_buf,"User #%d Runtime Statistic is here:\n",usrId);
        Say(str_buf);
        PrintStatInfo(usrStat);
    }
}

void PrintStatInfo(struct RuntimeSTAT* STT)
{
    printf("**********************************************\n");
    printf("BID=%d\tUID=%d\tTID=%d\t\n",STT->batchId,STT->userId,STT->traceId);
    printf("IO Mode:%s\n",STT->isWriteOnly?"WO":"RW");
    printf("Start LBA:%lu\n",STT->startLBA);
    printf("-------------------------------------\n");

    int hitRate = (double)STT->hitnum_s/STT->reqcnt_s * 100;
    printf("Hit Rate:\t%d%\n",hitRate);
    printf("Max CacheBlk:\t%lu\n", STT->cacheLimit);
    printf("Cur CacheBkl:\t%lu\n", STT->cacheUsage);

    printf("Req Count:\t%lu\n", STT->reqcnt_s);
    printf("Req Read:\t%lu\n", STT->reqcnt_r);
    printf("Req Write:\t%lu\n", STT->reqcnt_w);

    printf("Hit Sum:\t%lu\n", STT->hitnum_s);
    printf("Hit Read:\t%lu\n", STT->hitnum_r);
    printf("Hit Write:\t%lu\n", STT->hitnum_w);

    printf("read ssd blk:\t%lu\n", STT->load_ssd_blocks);
    printf("read hdd blk:\t%lu\n", STT->load_hdd_blocks);
    printf("write hdd bl:\t%lu\n", STT->flush_hdd_blocks);
    printf("write ssd bkl:\t%lu\n", STT->flush_ssd_blocks);
    printf("drop clean blk:\t%lu\n", STT->flush_clean_blocks);

    printf("time read ssd:\t%lf\n", STT->time_read_ssd);
    printf("time read hdd:\t%lf\n", STT->time_read_hdd);
    printf("time write ssd:\t%lf\n", STT->time_write_ssd);
    printf("time write hdd:\t%lf\n", STT->time_write_hdd);

    printf("HashMiss:\t%lu\n", STT->hashmiss_sum);
    printf("HashMiss Read:\t%lu\n", STT->hashmiss_read);
    printf("HashMiss Write:\t%lu\n", STT->hashmiss_write);
    printf("**********************************************\n\n");


}

void ResizeCache()
{
    struct RuntimeSTAT* usrStat;
    char* str_selectbatch = "Batch ID: ";
    char* str_selectuser = "User ID: ";
    char* str_nouser = "There is no the user\n";
    char* str_pipeerr = "something error...\n";

    char str_buf[1024];

    char str_bid[10];
    char str_uid[10];
    int batchId;
    int usrId;
    FILE* fd_shell;
    char command[1024];
    char shellbuf[1024];

    while(1)
    {
        Ask(str_selectbatch);
        if(readANS(str_bid,9)<0) return;
        batchId = atoi(str_bid);
        sprintf(command,"ls -l %s/STAT_b%d_* | grep \"^-\"|wc -l",PATH_SHM,batchId);

        fd_shell = popen(command,"r");
        if(fd_shell <=0 )
        {
            Say(str_pipeerr);
            continue;
        }
        fgets(shellbuf,1023,fd_shell);
        pclose(fd_shell);

        int usercnt = atoi(shellbuf);
        sprintf(str_buf,"%d Users in this batch.\n",usercnt);
        Say(str_buf);
        if(usercnt==0) continue;

        do
        {
            Ask(str_selectuser);
            if(readANS(str_uid,9)<0) return;
        }
        while(str_uid[0] == '\n');
        usrId = atoi(str_uid);
        sprintf(command,"ls %s/STAT_b%d_u%d*",PATH_SHM,batchId,usrId);

        fd_shell = popen(command,"r");
        if(fd_shell <= 0)
        {
            Say(str_pipeerr);
            continue;
        }
        fgets(shellbuf,1023,fd_shell);

        int len = strlen(shellbuf);
        if(len <= 0)
        {
            Say(str_nouser);
            continue;
        }
        shellbuf[len-1]=0;
        char* fname = strrchr(shellbuf,'/');
        fname++;

        usrStat = (struct RuntimeSTAT*)SHM_get(fname,sizeof(struct RuntimeSTAT));
        if(usrStat == NULL)
        {
            Say(str_pipeerr);
            continue;
        }

        sprintf(str_buf,"User #%d Runtime Statistic is here:\n",usrId);
        Say(str_buf);
        PrintStatInfo(usrStat);

        Ask("Re-Size the cahe limit to:");
        if(readANS(str_buf,64)<0) return;
        blksize_t cachelimit = atol(str_buf);
        usrStat->cacheLimit = cachelimit;
    }
}

void Ask(char* msg)
{
    printf("[monitor]# %s",msg);
}

void Say(char* msg)
{
    printf(msg);
}

static
int readANS(char* buf,int cnt)
{
    fgets(buf,cnt,stdin);
    if(buf[0] == 'q')
        return -1;
    return 0;
}
static
void ReadCommond()
{
    Ask("Command (h for help): ");
    fgets(cmdbuf,10,stdin);
    int cmd = cmdbuf[0];
    switch(cmd)
    {
    case 'a':
        CheckRuntime();
        break;
    case 'p':
        ResizeCache();
        break;
    case 'h':
        Say(str_help);
    }
}
