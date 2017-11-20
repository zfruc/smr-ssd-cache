#ifndef COSTMODEL_H
#define COSTMODEL_H
#include "global.h"
#include "cache.h"

typedef struct cm_token
{
	int will_evict_dirty_blkcnt;
	int will_evict_clean_blkcnt;
	double wtramp;
}cm_token;

extern int CM_Init();
extern int CM_Reg_EvictBlk(SSDBufTag blktag, unsigned flag);
extern int CM_CallBack(SSDBufTag blktag);
extern int CM_CHOOSE(cm_token token);
#endif // COSTMODEL_H


