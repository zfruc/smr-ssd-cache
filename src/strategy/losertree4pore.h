/*
    Author: Sun Diansen.    dssun@ruc.edu.cn
    Date: 2017.07.28

*/
#ifndef _LOSERTREE4OPORE_H_
#define _LOSERTREE4OPORE_H_
#include "pore.h"

typedef struct NonLeaf
{
    long value;
    int pathId;
    long despId;
}NonLeaf;

typedef struct LoserTreeInfo
{
    NonLeaf* non_leafs;
    int nonleaf_count;
    int winnerPathId;
    int maxValue;
}LoserTreeInfo;


extern int LoserTree_Create(int npath, StrategyDesp_pore* openBlkDesps,int maxValue, void** passport,int* winnerPathId, int* winnerDespId);
extern int LoserTree_GetWinner(void* passport, StrategyDesp_pore* candidateDesp, int* winnerPathId, int* winnerDespId);
extern int LoserTree_Destory(void* passport);

#endif // _LOSERTREE4OPORE_
