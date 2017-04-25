#define DEBUG 0
/* ---------------------------trace 2 call---------------------------- */
#ifndef TRACETOCALL
#define TRACETOCALL 1

#define ACT_READ '0'
#define ACT_WRITE '1'

#endif // TRACETOCALL

extern void trace_to_iocall(char* trace_file_path);
extern int BandOrBlock;
