#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "hrc.h"
extern int Fork_Pid;
char hrc_report_dir[] = "/tmp";

void hrc_usr_warning(const char* str)
{
    if(!I_AM_HRC_PROC){
        perror("hrc_report\n");
        exit(EXIT_FAILURE);
    }
    printf("HRC proc [%d] error: %s\n", Fork_Pid, str);
}


void hrc_report()
{
    if(!I_AM_HRC_PROC){
        perror("hrc_report\n");
        exit(EXIT_FAILURE);
    }
    void* str;
    posix_memalign(&str,512,512);

    char path[64];
    sprintf(path,"%s/hrc_%d",hrc_report_dir,Fork_Pid);
    int file = open(path, O_CREAT | O_RDWR | O_DIRECT | O_APPEND);
    int hitratio = (int)((float)STT->hitnum_s / STT->reqcnt_s * 100);

    if(file == -1){
        printf("hrc file open error: %s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }

    sprintf(str, "%d,%ld,%ld,%d\n", Fork_Pid, STT->reqcnt_s, STT->hitnum_s, hitratio);
//    printf("%s", (char*)str);
    ssize_t ret = pwrite(file,str,512,0);
//    printf("file handle = %d.ret of pwrite is %ld.\n",file,ret);
    if(ret < 512)
        hrc_usr_warning("report error");
    close(file);
}
