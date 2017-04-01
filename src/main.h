#define DEBUG 0
/* ---------------------------main---------------------------- */

extern char smr_device[100];
extern char ssd_device[100];
extern char inner_ssd_device[100];
extern int 		    hdd_fd;
extern int 		    ssd_fd;
extern int 		    inner_ssd_fd;
extern int BandOrBlock;
extern unsigned long NBLOCK_SSD_CACHE;
extern unsigned long NTABLE_SSD_CACHE;
extern unsigned long SSD_BUFFER_SIZE;
extern size_t ZONESZ;
extern unsigned long NSSDs;
extern unsigned long NSSDTables;
extern unsigned long NSSDLIMIT;
unsigned long PERIODTIMES;
