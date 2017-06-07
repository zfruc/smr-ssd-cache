#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "shmlib.h"
const char* PATH_SHM = "/dev/shm";

int
main(int argc, char** argv)
{
    CheckRuntime();
}

void CheckRuntime()
{
    struct RuntimeSTAT* usrStat;
    char* str_selectbatch = "Input the batch ID you've set: ";
    char* str_selectuser = "Input the user ID you want to check: ";
    char* str_nobatch = "There is no batch ID = %d\n";
    char* str_nouser = "There is no sser ID = %d\n";
    char* str_pipeerr = "something error...\n";

    char str_bid[10];
    char str_uid[10];
    int batchId;
    int usrId;
    FILE* fd_shell;
    char command[1024];
    char shellbuf[1024];

    while(1)
    {
        fputs(str_selectbatch,stdout);
        fgets(str_bid,9,stdin);
        batchId = atoi(str_bid);
        sprintf(command,"ls %s/STAT_b%d*",PATH_SHM,batchId);

        fd_shell = popen(command,"r");
        if(fd_shell <=0 )
        {
            printf(str_pipeerr);
            continue;
        }
        while(fgets(shellbuf,1023,fd_shell)>0)
        {
            printf(shellbuf);
        }
        pclose(fd_shell);

        fputs(str_selectuser,stdout);
        fgets(str_uid,9,stdin);
        usrId = atoi(str_uid);
        sprintf(command,"ls %s/STAT_b%d_u%d*",PATH_SHM,batchId,usrId);

        fd_shell = popen(command,"r");
        if(fd_shell <= 0)
        {
            printf(str_pipeerr);
            continue;
        }
        fgets(shellbuf,1023,fd_shell);

        int len = strlen(shellbuf);
        if(len <= 0)
        {
            printf(str_nouser);
            continue;
        }
        shellbuf[len-1]=0;
        char* fname = strrchr(shellbuf,'/');
        fname++;

        usrStat = (struct RuntimeSTAT*)SHM_get(fname,sizeof(struct RuntimeSTAT));
        if(usrStat == NULL)
        {
            printf(str_pipeerr);
            continue;
        }

        printf("User [%d] Runtime Statistic is here:\n",usrId);
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

