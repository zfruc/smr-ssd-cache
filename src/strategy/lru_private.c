#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "lru.h"
#include "ssd-cache.h"
#include "lru_private.h"
#include "shmlib.h"
/********
 ** SHM**
 ********/

static StrategyCtrl_LRU_private *strategy_ctrl;
static StrategyDesp_LRU_private	*strategy_desp;
static StrategyCtrl_LRU_private *self_strategy_ctrl;

static volatile void *addToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static volatile void *deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static int hasDeleted(StrategyDesp_LRU_private* ssd_buf_hdr_for_lru);
/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferFor_LRU_private()
{
    STT->cacheLimit = Param1;
    int stat = SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        strategy_ctrl =(StrategyCtrl_LRU_private *)SHM_alloc(SHM_SSDBUF_STRATEGY_CTRL,sizeof(StrategyCtrl_LRU_private));
        strategy_desp = (StrategyDesp_LRU_private *)SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_private) * NBLOCK_SSD_CACHE);

        strategy_ctrl->first_lru = -1;
        strategy_ctrl->last_lru = -1;
        SHM_mutex_init(&strategy_ctrl->lock);

        StrategyDesp_LRU_private *ssd_buf_hdr_for_lru = strategy_desp;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->serial_id = i;
            ssd_buf_hdr_for_lru->next_lru = -1;
            ssd_buf_hdr_for_lru->last_lru = -1;
            ssd_buf_hdr_for_lru->next_self_lru = -1;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            SHM_mutex_init(&
                           ssd_buf_hdr_for_lru->lock);
        }
    }
    else
    {
        strategy_ctrl =(StrategyCtrl_LRU_private *)SHM_get(SHM_SSDBUF_STRATEGY_CTRL,sizeof(StrategyCtrl_LRU_private));
        strategy_desp = (StrategyDesp_LRU_private *)SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_private) * NBLOCK_SSD_CACHE);

    }
    SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");
    self_strategy_ctrl = (StrategyCtrl_LRU_private *)malloc(sizeof(StrategyCtrl_LRU_private));
    self_strategy_ctrl->first_lru = -1;
    self_strategy_ctrl->last_lru = -1;
    return stat;
}

long
Unload_Buf_LRU_private()
{
    _LOCK(&strategy_ctrl->lock);

    long frozen_id = self_strategy_ctrl->last_lru;
    deleteFromLRU(&strategy_desp[frozen_id]);

    _UNLOCK(&strategy_ctrl->lock);
    STT->cacheUsage--;
    return frozen_id;
}

int
hitInBuffer_LRU_private(long serial_id)
{
    _LOCK(&strategy_ctrl->lock);

    StrategyDesp_LRU_private* ssd_buf_hdr_for_lru = &strategy_desp[serial_id];
    if(hasBeenDeleted(ssd_buf_hdr_for_lru))
    {
        _UNLOCK(&strategy_ctrl->lock);
        return -1;
    }
    moveToLRUHead(ssd_buf_hdr_for_lru);
    _UNLOCK(&strategy_ctrl->lock);

    return 0;
}

void*
insertBuffer_LRU_private(long serial_id)
{
    _LOCK(&strategy_ctrl->lock);

    strategy_desp[serial_id].user_id = UserId;
    addToLRUHead(&strategy_desp[serial_id]);

    _UNLOCK(&strategy_ctrl->lock);
    STT->cacheUsage++;
    return 0;
}

int
isOverUpperLimit()
{
    if(STT->cacheUsage >= STT->cacheLimit)
        return 1;
}

static volatile void *
addToLRUHead(StrategyDesp_LRU_private* ssd_buf_hdr_for_lru)
{
    if (strategy_ctrl->last_lru < 0)
    {
        strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->serial_id;
        strategy_ctrl->last_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    else
    {
        ssd_buf_hdr_for_lru->next_lru = strategy_desp[strategy_ctrl->first_lru].serial_id;
        ssd_buf_hdr_for_lru->last_lru = -1;
        strategy_desp[strategy_ctrl->first_lru].last_lru = ssd_buf_hdr_for_lru->serial_id;
        strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->serial_id;
    }
    //deal with self LRU queue
    if(ssd_buf_hdr_for_lru->user_id == UserId)
    {
        if(self_strategy_ctrl->last_lru < 0)
        {
            self_strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->serial_id;
            self_strategy_ctrl->last_lru = ssd_buf_hdr_for_lru->serial_id;
        }
        else
        {
            ssd_buf_hdr_for_lru->next_self_lru = strategy_desp[self_strategy_ctrl->first_lru].serial_id;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            strategy_desp[self_strategy_ctrl->first_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
            self_strategy_ctrl->first_lru =  ssd_buf_hdr_for_lru->serial_id;

        }
    }
    return NULL;
}

static volatile void *
deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru)
{
    if (ssd_buf_hdr_for_lru->last_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->last_lru].next_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    else
    {
        strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    //deal with self queue
    if(ssd_buf_hdr_for_lru->user_id == UserId)
    {
        if(ssd_buf_hdr_for_lru->last_self_lru>=0)
        {
            strategy_desp[ssd_buf_hdr_for_lru->last_self_lru].next_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
        }
        else
        {
            self_strategy_ctrl->first_lru = ssd_buf_hdr_for_lru->next_self_lru;
        }
    }
    if (ssd_buf_hdr_for_lru->next_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->next_lru].last_lru = ssd_buf_hdr_for_lru->last_lru;
    }
    else
    {
        strategy_ctrl->last_lru = ssd_buf_hdr_for_lru->last_lru;
    }
    //deal with self queue
    if(ssd_buf_hdr_for_lru->user_id == UserId)
    {
        if(ssd_buf_hdr_for_lru->next_self_lru>=0)
        {
            strategy_desp[ssd_buf_hdr_for_lru->next_self_lru].last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
        }
        else
        {
            self_strategy_ctrl->last_lru = ssd_buf_hdr_for_lru->last_self_lru;
        }
    }

    ssd_buf_hdr_for_lru->last_lru = ssd_buf_hdr_for_lru->next_lru = -1;

    return NULL;
}

static volatile void *
moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru);
    return NULL;
}

static int
hasDeleted(StrategyDesp_LRU_private* ssd_buf_hdr_for_lru)
{
    if(ssd_buf_hdr_for_lru->last_lru < 0 && ssd_buf_hdr_for_lru->next_lru < 0)
        return 1;
    else
        return 0;
}
