#include <stdio.h>
#include <stdlib.h>
#include "ssd-cache.h"
#include "smr-simulator/smr-simulator.h"
#include "lru.h"
#include <shmlib.h>

static volatile void *addToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru);
static volatile void *deleteFromLRU(SSDBufDespForLRU * ssd_buf_hdr_for_lru);
static volatile void *moveToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru);

/*
 * init buffer hash table, strategy_control, buffer, work_mem
 */
int
initSSDBufferForLRU()
{
    int stat = SHM_lock_n_check("LOCK_SSDBUF_STRATEGY_LRU");
    if(stat == 0)
    {
        ssd_buf_strategy_ctrl_lru =(SSDBufferStrategyControlForLRU *)SHM_alloc(SHM_SSDBUF_STRATEGY_CTRL,sizeof(SSDBufferStrategyControlForLRU));
        ssd_buf_desp_for_lru = (SSDBufDespForLRU *)SHM_alloc(SHM_SSDBUF_STRATEGY_DESP, sizeof(SSDBufDespForLRU) * NBLOCK_SSD_CACHE);

        ssd_buf_strategy_ctrl_lru->first_lru = -1;
        ssd_buf_strategy_ctrl_lru->last_lru = -1;
        SHM_mutex_init(&ssd_buf_strategy_ctrl_lru->lock);

        SSDBufDespForLRU *ssd_buf_hdr_for_lru = ssd_buf_desp_for_lru;
        long i;
        for (i = 0; i < NBLOCK_SSD_CACHE; ssd_buf_hdr_for_lru++, i++)
        {
            ssd_buf_hdr_for_lru->ssd_buf_id = i;
            ssd_buf_hdr_for_lru->next_lru = -1;
            ssd_buf_hdr_for_lru->last_lru = -1;
        }
    }
    else
    {
        ssd_buf_strategy_ctrl_lru =(SSDBufferStrategyControlForLRU *)SHM_get(SHM_SSDBUF_STRATEGY_CTRL,sizeof(SSDBufferStrategyControlForLRU));
        ssd_buf_desp_for_lru = (SSDBufDespForLRU *)SHM_get(SHM_SSDBUF_STRATEGY_DESP, sizeof(SSDBufDespForLRU) * NBLOCK_SSD_CACHE);

    }
    SHM_unlock("LOCK_SSDBUF_STRATEGY_LRU");
    return stat;
}

static volatile void *
addToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru)
{
    if (ssd_buf_desp_ctrl->n_usedssd == 0)
    {
        ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->ssd_buf_id;
        ssd_buf_strategy_ctrl_lru->last_lru = ssd_buf_hdr_for_lru->ssd_buf_id;
    }
    else
    {
        ssd_buf_hdr_for_lru->next_lru = ssd_buf_desp_for_lru[ssd_buf_strategy_ctrl_lru->first_lru].ssd_buf_id;
        ssd_buf_hdr_for_lru->last_lru = -1;
        ssd_buf_desp_for_lru[ssd_buf_strategy_ctrl_lru->first_lru].last_lru = ssd_buf_hdr_for_lru->ssd_buf_id;
        ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->ssd_buf_id;
    }
    return NULL;
}

static volatile void *
deleteFromLRU(SSDBufDespForLRU * ssd_buf_hdr_for_lru)
{
    if (ssd_buf_hdr_for_lru->last_lru >= 0)
    {
        ssd_buf_desp_for_lru[ssd_buf_hdr_for_lru->last_lru].next_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    else
    {
        ssd_buf_strategy_ctrl_lru->first_lru = ssd_buf_hdr_for_lru->next_lru;
    }
    if (ssd_buf_hdr_for_lru->next_lru >= 0)
    {
        ssd_buf_desp_for_lru[ssd_buf_hdr_for_lru->next_lru].last_lru = ssd_buf_hdr_for_lru->last_lru;
    }
    else
    {
        ssd_buf_strategy_ctrl_lru->last_lru = ssd_buf_hdr_for_lru->last_lru;
    }

    return NULL;
}

static volatile void *
moveToLRUHead(SSDBufDespForLRU * ssd_buf_hdr_for_lru)
{
    deleteFromLRU(ssd_buf_hdr_for_lru);
    addToLRUHead(ssd_buf_hdr_for_lru);

    return NULL;
}

SSDBufDesp  *
getLRUBuffer()
{
    SSDBufDesp  *ssd_buf_hdr;
    SSDBufDespForLRU *ssd_buf_hdr_for_lru;

    if (ssd_buf_desp_ctrl->first_freessd >= 0)
    {
        ssd_buf_hdr = &ssd_buf_desps[ssd_buf_desp_ctrl->first_freessd];
        ssd_buf_hdr_for_lru = &ssd_buf_desp_for_lru[ssd_buf_desp_ctrl->first_freessd];
        ssd_buf_desp_ctrl->first_freessd = ssd_buf_hdr->next_freessd;
        ssd_buf_hdr->next_freessd = -1;
        addToLRUHead(ssd_buf_hdr_for_lru);
        ssd_buf_desp_ctrl->n_usedssd++;
        return ssd_buf_hdr;
    }

    /** When there is NO free SSD space for cache, then TODO flush **/
    long coldest_id = ssd_buf_strategy_ctrl_lru->last_lru;
    ssd_buf_hdr = &ssd_buf_desps[coldest_id];
    ssd_buf_hdr_for_lru = &ssd_buf_desp_for_lru[coldest_id];

    unsigned char	old_flag = ssd_buf_hdr->ssd_buf_flag;
    SSDBufferTag	old_tag = ssd_buf_hdr->ssd_buf_tag;
    if (DEBUG)
        printf("[INFO] SSDBufferAlloc(): old_flag&SSD_BUF_DIRTY=%d\n", old_flag & SSD_BUF_DIRTY);
    if ((old_flag & SSD_BUF_DIRTY) != 0)
    {
        flushSSDBuffer(ssd_buf_hdr);
    }
//    if ((old_flag & SSD_BUF_VALID) != 0)
//    {
//        unsigned long	old_hash = ssdbuftableHashcode(&old_tag);
//        ssdbuftableDelete(&old_tag, old_hash);
//    }

    moveToLRUHead(ssd_buf_hdr_for_lru);

    return ssd_buf_hdr;
}

void*
hitInLRUBuffer(SSDBufDesp * ssd_buf_hdr)
{
    moveToLRUHead(&ssd_buf_desp_for_lru[ssd_buf_hdr->ssd_buf_id]);
    return NULL;
}
