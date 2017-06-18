#define DEBUG 0
/* ---------------------------ssd cache---------------------------- */
#ifndef _SSD_CACHE_H
#define _SSD_CACHE_H 1

#include <pthread.h>
#include "global.h"
#include "timerUtils.h"

#define bool    unsigned char

typedef struct
{
    off_t	offset;
} SSDBufferTag;

typedef struct
{
    long        serial_id;              // the serial number of the descriptor corresponding to SSD buffer.
    long 		ssd_buf_id;				// SSD buffer location in shared buffer
    unsigned 	ssd_buf_flag;
    long		next_freessd;           // to link the desp serial number of free SSD buffer
    SSDBufferTag 	ssd_buf_tag;
    pthread_mutex_t lock;               // For the fine grain size
} SSDBufDesp;

#define SSD_BUF_VALID 0x01
#define SSD_BUF_DIRTY 0x02


typedef struct
{
    long		n_usedssd;			// For eviction
    long		first_freessd;		// Head of list of free ssds
    pthread_mutex_t lock;
} SSDBufDespCtrl;

extern int IsHit;
extern microsecond_t msec_r_hdd,msec_w_hdd,msec_r_ssd,msec_w_ssd;

extern void initSSD();
extern void read_block(off_t offset, char* ssd_buffer);
extern void write_block(off_t offset, char* ssd_buffer);
extern void read_band(off_t offset, char* ssd_buffer);
extern void write_band(off_t offset, char* ssd_buffer);
extern bool isSamebuf(SSDBufferTag *, SSDBufferTag *);
extern int ResizeCacheUsage();
extern void CopySSDBufTag(SSDBufferTag* objectTag, SSDBufferTag* sourceTag);

extern void _LOCK(pthread_mutex_t* lock);
extern void _UNLOCK(pthread_mutex_t* lock);

#endif
