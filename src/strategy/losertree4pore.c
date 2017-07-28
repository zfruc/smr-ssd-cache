#include "losertree4pore.h"
#include <math.h>

static int lg2_above(int V)
{
    V--;
    V |= V >> 1;
    V |= V >> 2;
    V |= V >> 4;
    V |= V >> 8;
    V |= V >> 16;
    V++;

    int count = 0;
    while(V!=1)
    {
        V = V >> 1;
        count++;
    }
    return count;
}

static void adjust(LoserTreeInfo* info, StrategyDesp_pore* winnerDesp)
{
    NonLeaf winnerLeaf, tmpLeaf;
    winnerLeaf.value = winnerDesp->stamp;
    winnerLeaf.pathId = info->winnerPathId;
    winnerLeaf.despId = winnerDesp->serial_id;

    int parentId = (info->nonleaf_count + info->winnerPathId)/2;
    while(parentId>0)
    {
        NonLeaf* parentLeaf = info->non_leafs + parentId;
        if(winnerLeaf.value > parentLeaf->value){
            //winner lose, parent win
            tmpLeaf = *parentLeaf;
            *parentLeaf = winnerLeaf;
            winnerLeaf = tmpLeaf;
        }
        parentId /= 2;
    }
    info->non_leafs[0] = winnerLeaf;
    info->winnerPathId = winnerLeaf.pathId;
}

int LoserTree_Create(int npath, StrategyDesp_pore* openBlkDesps,int maxValue, void** passport,int* winnerPathId, int* winnerDespId)
{
    int nlevels = lg2_above(npath);
    int nodes_count = pow(2,nlevels);
    NonLeaf* non_leafs = (NonLeaf*)malloc(sizeof(NonLeaf)*nodes_count);

    LoserTreeInfo* ltInfo = (LoserTreeInfo*)malloc(sizeof(LoserTreeInfo));
    ltInfo->maxValue = maxValue +1;
    ltInfo->nonleaf_count = nodes_count;
    ltInfo->non_leafs = non_leafs;
    ltInfo->winnerPathId = 0;

    /** initial the loser tree with the min value of each path **/
    int i;
    for(i = 0; i < nodes_count; i++){
        non_leafs[i].pathId = -1;
        non_leafs[i].value = -1;
    }

    for(i = 0; i < nodes_count; i++){
        ltInfo->winnerPathId = i;
        adjust(ltInfo,openBlkDesps + i);
    }

    winnerPathId = ltInfo->non_leafs[0].pathId;
    winnerDespId = ltInfo->non_leafs[0].despId;
    *passport = ltInfo;

    return ltInfo->non_leafs[0].value;
}

/** \brief
 * Get the next one winner.
 * \param passport: which was given when initialization.
 * \param candidateDesp:
    the candidate block descriptor which was decieded according the pathId which the last winner descriptor belongs to.
    Usually choosing the tail descriptor in the winner path(Zone). If the path(zone) empty, just give the MAXVALUE descriptor.
 * \param winnerPathId: return the winner path ID this round.
 * \param winnerDespId: return the winner desp ID this round.
 * \return
 *  0: no error.
 */
int
LoserTree_GetWinner(void* passport, StrategyDesp_pore* candidateDesp, int* winnerPathId, int* winnerDespId)
{
    LoserTreeInfo* ltInfo = (LoserTreeInfo*)passport;
    adjust(ltInfo,candidateDesp);
    *winnerPathId = ltInfo->non_leafs[0].pathId;
    *winnerDespId = ltInfo->non_leafs[0].despId;
    return ltInfo->non_leafs[0].value;
}

int LoserTree_Destory(void* passport)
{
    LoserTreeInfo* info =(LoserTreeInfo*)passport;
    int i = 0;
    while(i < info->nonleaf_count){
        free(info->non_leafs + i);
    }
    free(info);
    return 0;
}
