#include <stdio.h>
#include <stdlib.h>

#include "global.h"
#include "lru.h"
#include "cache.h"
#include "lru_private.h"
#include "shmlib.h"
/********
 ** SHM**
 ********/

static StrategyCtrl_LRU_private lru_dirty_ctrl, lru_clean_ctrl; //*self_strategy_ctrl,
static StrategyDesp_LRU_private	* strategy_desp;

static volatile void *addToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru, unsigned flag);
static volatile void *deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru, unsigned flag);

#define IsDirty(flag) ((flag & SSD_BUF_DIRTY) != 0)

/*
 * init buffer hash table, Strategy_control, buffer, work_mem
 */
int
initSSDBufferFor_LRU_rw()
{
    //STT->cacheLimit = Param1;
    int stat = multi_SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        strategy_desp = (StrategyDesp_LRU_private *)multi_SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_private) * NBLOCK_SSD_CACHE);

        StrategyDesp_LRU_private *ssd_buf_hdr_for_lru = strategy_desp;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->serial_id = i;
            ssd_buf_hdr_for_lru->next_self_lru = -1;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            multi_SHM_mutex_init(&ssd_buf_hdr_for_lru->lock);
        }
    }
    else
    {
        strategy_desp = (StrategyDesp_LRU_private *)multi_SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(StrategyDesp_LRU_private) * NBLOCK_SSD_CACHE);
    }
    multi_SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");

    lru_dirty_ctrl.first_self_lru = lru_clean_ctrl.first_self_lru = -1;
    lru_dirty_ctrl.last_self_lru = lru_clean_ctrl.last_self_lru = -1;
    lru_dirty_ctrl.count = lru_clean_ctrl.count = 0;

    return stat;
}

long
Unload_Buf_LRU_rw(unsigned flag)
{
    long frozen_id;
    if (IsDirty(flag))
    {
        if(lru_dirty_ctrl.last_self_lru >= 0)
            frozen_id = lru_dirty_ctrl.last_self_lru;
        else
            frozen_id = lru_clean_ctrl.last_self_lru;
    }
    else
    {
        if(lru_clean_ctrl.last_self_lru >= 0)
            frozen_id = lru_clean_ctrl.last_self_lru;
        else
            frozen_id = lru_dirty_ctrl.last_self_lru;
    }
    if(frozen_id < 0)
        exit(-1);
    deleteFromLRU(&strategy_desp[frozen_id]);

    return frozen_id;
}

int
hitInBuffer_LRU_rw(long serial_id, unsigned flag)
{
    StrategyDesp_LRU_private* ssd_buf_hdr_for_lru = &strategy_desp[serial_id];
    moveToLRUHead(ssd_buf_hdr_for_lru, flag);
    return 0;
}

int
insertBuffer_LRU_rw(long serial_id, unsigned flag)
{
    if(serial_id == 5249)
    {
        int a = 0;
    }
    //strategy_desp[serial_id].user_id = UserId;
    addToLRUHead(&strategy_desp[serial_id], flag);

    return 0;
}

static volatile void *
addToLRUHead(StrategyDesp_LRU_private* ssd_buf_hdr_for_lru, unsigned flag)
{
    //deal with self LRU queue
    if (IsDirty(flag))
    {
        if(lru_dirty_ctrl.first_self_lru < 0)
        {   // empty
            lru_dirty_ctrl.first_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_dirty_ctrl.last_self_lru = ssd_buf_hdr_for_lru->serial_id;
        }
        else
        {
            ssd_buf_hdr_for_lru->next_self_lru = lru_dirty_ctrl.first_self_lru;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            strategy_desp[lru_dirty_ctrl.first_self_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_dirty_ctrl.first_self_lru =  ssd_buf_hdr_for_lru->serial_id;
        }
    }
    else
    {
        if(lru_clean_ctrl.first_self_lru < 0)
        {   // empty
            lru_clean_ctrl.first_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_clean_ctrl.last_self_lru = ssd_buf_hdr_for_lru->serial_id;
        }
        else
        {
            ssd_buf_hdr_for_lru->next_self_lru = lru_clean_ctrl.first_self_lru;
            ssd_buf_hdr_for_lru->last_self_lru = -1;
            strategy_desp[lru_clean_ctrl.first_self_lru].last_self_lru = ssd_buf_hdr_for_lru->serial_id;
            lru_clean_ctrl.first_self_lru =  ssd_buf_hdr_for_lru->serial_id;
        }

    }

    return NULL;
}

static volatile void *
deleteFromLRU(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru)
{
    //deal with self queue
    if(ssd_buf_hdr_for_lru->last_self_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->last_self_lru].next_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
    }
    else
    {
        if(lru_clean_ctrl.first_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_clean_ctrl.first_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
        else if (lru_dirty_ctrl.first_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_dirty_ctrl.first_self_lru = ssd_buf_hdr_for_lru->next_self_lru;
        else
            exit(-1);
    }

    if(ssd_buf_hdr_for_lru->next_self_lru >= 0)
    {
        strategy_desp[ssd_buf_hdr_for_lru->next_self_lru].last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
    }
    else
    {
        if(lru_clean_ctrl.last_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_clean_ctrl.last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
        else if (lru_dirty_ctrl.last_self_lru == ssd_buf_hdr_for_lru->serial_id)
            lru_dirty_ctrl.last_self_lru = ssd_buf_hdr_for_lru->last_self_lru;
        else
            exit(-1);
    }

    ssd_buf_hdr_for_lru->last_self_lru = ssd_buf_hdr_for_lru->next_self_lru = -1;

    return NULL;
}

static volatile void *
moveToLRUHead(StrategyDesp_LRU_private * ssd_buf_hdr_for_lru, unsigned flag)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru, flag);
    return NULL;
}


