#ifndef COSTMODEL_H
#define COSTMODEL_H
#include "global.h"
#include "cache.h"


extern int CM_Init();
extern int CM_Reg_EvictBlk(SSDBufTag blktag, unsigned flag);
extern int CM_CHOOSE();
#endif // COSTMODEL_H

