#include <stdlib.h>
#include "pv3.h"
#include "../statusDef.h"
#include "../report.h"
#include "costmodel.h"
#include <math.h>
//#define random(x) (rand()%x)
#define IsDirty(flag) ( (flag & SSD_BUF_DIRTY) != 0 )
#define IsClean(flag) ( (flag & SSD_BUF_DIRTY) == 0 )

#define EVICT_DITRY_GRAIN 64 // The grain of once dirty blocks eviction

typedef struct
{
    long            pagecnt_clean;
    long            head,tail;
    pthread_mutex_t lock;
} CleanDespCtrl;

static blkcnt_t  ZONEBLKSZ;

static StrategyDesp_pore*   GlobalDespArray;
static ZoneCtrl*            ZoneCtrlArray;
static CleanDespCtrl        CleanCtrl;

static unsigned long*       ZoneSortArray;      /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int                  NonEmptyZoneCnt = 0;
static unsigned long*       OpenZoneSet;        /* The decided open zones in current period, which chosed by both the weight-sorted array and the access threshold. */
static int                  OpenZoneCnt;        /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

extern long                 Cycle_Length;        /* Which defines the upper limit of the block amount of selected OpenZone and of Evicted blocks. */
static long                 Cycle_Progress;     /* Current times to evict clean/dirty block in a period lenth */
static long                 StampGlobal;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */
static long                 CycleID;

static void add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void move2ArrayHead(StrategyDesp_pore* desp,ZoneCtrl* zoneCtrl);

static int start_new_cycle();
static void stamp(StrategyDesp_pore * desp);

static void unloadfromZone(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void clearDesp(StrategyDesp_pore* desp);
static void hit(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void add2CleanArrayHead(StrategyDesp_pore* desp);
static void unloadfromCleanArray(StrategyDesp_pore* desp);
static void move2CleanArrayHead(StrategyDesp_pore* desp);

/** PAUL**/
static int redefineOpenZones();
static int get_FrozenOpZone_Seq();
static int random_pick(float weight1, float weight2, float obey);

static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
Init_poreplus_v3()
{
    ZONEBLKSZ = ZONESZ / BLKSZ;

    CycleID = StampGlobal = Cycle_Progress = 0;
    GlobalDespArray = (StrategyDesp_pore*)malloc(sizeof(StrategyDesp_pore) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl*)malloc(sizeof(ZoneCtrl) * NZONES);

    NonEmptyZoneCnt = OpenZoneCnt = 0;
    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    OpenZoneSet = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);
    int i = 0;
    while(i < NBLOCK_SSD_CACHE)
    {
        StrategyDesp_pore* desp = GlobalDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->heat = 0;
        desp->stamp = 0;
        desp->flag = 0;
        desp->zoneId = -1;
        i++;
    }
    i = 0;
    while(i < NZONES)
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + i;
        ctrl->zoneId = i;
        ctrl->heat = ctrl->pagecnt_clean = ctrl->pagecnt_dirty = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->score = 0;
        ZoneSortArray[i] = 0;
        i++;
    }
    CleanCtrl.pagecnt_clean = 0;
    CleanCtrl.head = CleanCtrl.tail = -1;
    return 0;
}

int
LogIn_poreplus_v3(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    StrategyDesp_pore* myDesp = GlobalDespArray + despId;
    unsigned long myZoneId = getZoneNum(tag.offset);
    ZoneCtrl* myZone = ZoneCtrlArray + myZoneId;
    myDesp->zoneId = myZoneId;
    myDesp->ssd_buf_tag = tag;
    myDesp->flag |= flag;

    /* add into chain */
    stamp(myDesp);

    if(IsDirty(flag))
    {
        /* add into Zone LRU as it's dirty tag */
        add2ArrayHead(myDesp, myZone);
        myZone->pagecnt_dirty++;
        //myZone->score ++ ;
    }
    else
    {
        /* add into Global Clean LRU as it's clean tag */
        add2CleanArrayHead(myDesp);
        CleanCtrl.pagecnt_clean++;
    }

    return 1;
}

int
Hit_poreplus_v3(long despId, unsigned flag)
{
    StrategyDesp_pore* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(myDesp->ssd_buf_tag.offset);

    if (IsClean(myDesp->flag) && IsDirty(flag))
    {
        /* clean --> dirty */
        unloadfromCleanArray(myDesp);
        add2ArrayHead(myDesp,myZone);
        myZone->pagecnt_dirty++;
        CleanCtrl.pagecnt_clean--;
        hit(myDesp,myZone);
    }
    else if (IsClean(myDesp->flag) && IsClean(flag))
    {
        /* clean --> clean */
        move2CleanArrayHead(myDesp);
    }
    else
    {
        /* dirty hit again*/
        move2ArrayHead(myDesp,myZone);
        hit(myDesp,myZone);
    }
    stamp(myDesp);
    myDesp->flag |= flag;

    return 0;
}

static int
start_new_cycle()
{
    CycleID++;
    Cycle_Progress = 0;

    int cnt = redefineOpenZones();

    printf("-------------New Cycle!-----------\n");
    printf("Cycle ID [%ld], Non-Empty Zone_Cnt=%d, OpenZones_cnt=%d, CleanBlks=%ld(%0.2lf)\n",CycleID, NonEmptyZoneCnt, OpenZoneCnt,CleanCtrl.pagecnt_clean, (double)CleanCtrl.pagecnt_clean/NBLOCK_SSD_CACHE);

    return cnt;
}

/** \brief
 */
int
LogOut_poreplus_v3(long * out_despid_array, int max_n_batch, enum_t_vict suggest_type)
{
    static int CurEvictZoneSeq;
    static long n_evict_clean_cycle = 0, n_evict_dirty_cycle = 0;
    int evict_cnt = 0;

    ZoneCtrl* evictZone;

    if(suggest_type == ENUM_B_Clean)
    {
        if(CleanCtrl.pagecnt_clean == 0) // Consistency judgment
            usr_error("Order to evict clean cache block, but it is exhausted in advance.");
        goto FLAG_EVICT_CLEAN;
    }
    else if(suggest_type == ENUM_B_Dirty)
    {
        if(STT->incache_n_dirty == 0)   // Consistency judgment
            usr_error("Order to evict dirty cache block, but it is exhausted in advance.");
        goto FLAG_EVICT_DIRTYZONE;
    }
    else if(suggest_type == ENUM_B_Any)
    {
        if(STT->incache_n_clean == 0)
            goto FLAG_EVICT_DIRTYZONE;
        else if(STT->incache_n_dirty == 0)
            goto FLAG_EVICT_CLEAN;
        else
        {
            int it = random_pick((float)STT->incache_n_clean, (float)STT->incache_n_dirty, 1);
            if(it == 1)
                goto FLAG_EVICT_CLEAN;
            else
                goto FLAG_EVICT_DIRTYZONE;
        }
    }
    else
        usr_error("PAUL doesn't support this evict type.");

FLAG_EVICT_CLEAN:
    while(evict_cnt < EVICT_DITRY_GRAIN && CleanCtrl.pagecnt_clean > 0)
    {
        StrategyDesp_pore * cleanDesp = GlobalDespArray + CleanCtrl.tail;
        out_despid_array[evict_cnt] = cleanDesp->serial_id;
        unloadfromCleanArray(cleanDesp);
        clearDesp(cleanDesp);

        n_evict_clean_cycle ++;
        CleanCtrl.pagecnt_clean --;
        evict_cnt ++;
    }
    return evict_cnt;

FLAG_EVICT_DIRTYZONE:
    if(Cycle_Progress >= Cycle_Length || Cycle_Progress == 0){
        start_new_cycle();

        n_evict_clean_cycle = n_evict_dirty_cycle = 0;
        printf("Ouput of last Cycle: clean:%ld, dirty:%ld\n",n_evict_clean_cycle,n_evict_dirty_cycle);
    }

    CurEvictZoneSeq = get_FrozenOpZone_Seq();
    evictZone = ZoneCtrlArray + OpenZoneSet[CurEvictZoneSeq];

    while(evict_cnt < EVICT_DITRY_GRAIN && evictZone->pagecnt_dirty > 0)
    {
        StrategyDesp_pore* frozenDesp = GlobalDespArray + evictZone->tail;

        unloadfromZone(frozenDesp,evictZone);
        out_despid_array[evict_cnt] = frozenDesp->serial_id;
//        evictZone->score -= (double) 1 / (1 << frozenDesp->heat);

        Cycle_Progress ++;
        evictZone->pagecnt_dirty--;
        evictZone->heat -= frozenDesp->heat;
        n_evict_dirty_cycle++;

        clearDesp(frozenDesp);
        evict_cnt++;
    }
    //printf("pore+V2: batch flush dirty cnt [%d] from zone[%lu]\n", j,evictZone->zoneId);

//    printf("SCORE REPORT: zone id[%d], score[%lu]\n", evictZone->zoneId, evictZone->score);
    return evict_cnt;
}

/****************
** Utilities ****
*****************/
/* Utilities for Dirty descriptors Array in each Zone*/

static void
hit(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    desp->heat ++;
    zoneCtrl->heat++;
//    zoneCtrl->score -= (double) 1 / (1 << desp->heat);
}

static void
add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(zoneCtrl->head < 0)
    {
        //empty
        zoneCtrl->head = zoneCtrl->tail = desp->serial_id;
    }
    else
    {
        //unempty
        StrategyDesp_pore* headDesp = GlobalDespArray + zoneCtrl->head;
        desp->pre = -1;
        desp->next = zoneCtrl->head;
        headDesp->pre = desp->serial_id;
        zoneCtrl->head = desp->serial_id;
    }
}

static void
unloadfromZone(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(desp->pre < 0)
    {
        zoneCtrl->head = desp->next;
    }
    else
    {
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0)
    {
        zoneCtrl->tail = desp->pre;
    }
    else
    {
        GlobalDespArray[desp->next].pre = desp->pre;
    }
    desp->pre = desp->next = -1;
}

static void
move2ArrayHead(StrategyDesp_pore* desp,ZoneCtrl* zoneCtrl)
{
    unloadfromZone(desp, zoneCtrl);
    add2ArrayHead(desp, zoneCtrl);
}

static void
clearDesp(StrategyDesp_pore* desp)
{
    desp->ssd_buf_tag.offset = -1;
    desp->next = desp->pre = -1;
    desp->heat = 0;
    desp->stamp = 0;
    desp->flag &= ~(SSD_BUF_DIRTY | SSD_BUF_VALID);
    desp->zoneId = -1;
}

/* Utilities for Global Clean Descriptors Array */
static void
add2CleanArrayHead(StrategyDesp_pore* desp)
{
    if(CleanCtrl.head < 0)
    {
        //empty
        CleanCtrl.head = CleanCtrl.tail = desp->serial_id;
    }
    else
    {
        //unempty
        StrategyDesp_pore* headDesp = GlobalDespArray + CleanCtrl.head;
        desp->pre = -1;
        desp->next = CleanCtrl.head;
        headDesp->pre = desp->serial_id;
        CleanCtrl.head = desp->serial_id;
    }
}

static void
unloadfromCleanArray(StrategyDesp_pore* desp)
{
    if(desp->pre < 0)
    {
        CleanCtrl.head = desp->next;
    }
    else
    {
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0)
    {
        CleanCtrl.tail = desp->pre;
    }
    else
    {
        GlobalDespArray[desp->next].pre = desp->pre;
    }
    desp->pre = desp->next = -1;
}

static void
move2CleanArrayHead(StrategyDesp_pore* desp)
{
    unloadfromCleanArray(desp);
    add2CleanArrayHead(desp);
}

/* Decision Method */
/** \brief
 *  Quick-Sort method to sort the zones by score.
    NOTICE!
        If the gap between variable 'start' and 'end', it will PROBABLY cause call stack OVERFLOW!
        So this function need to modify for better.
 */
static void
qsort_zone(long start, long end)
{
    long		i = start;
    long		j = end;

    long S = ZoneSortArray[start];
    ZoneCtrl* curCtrl = ZoneCtrlArray + S;
    unsigned long sScore = curCtrl->score;
    while (i < j)
    {
        while (!(ZoneCtrlArray[ZoneSortArray[j]].score > sScore) && i<j)
        {
            j--;
        }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (!(ZoneCtrlArray[ZoneSortArray[i]].score < sScore) && i<j)
        {
            i++;
        }
        ZoneSortArray[j] = ZoneSortArray[i];
    }

    ZoneSortArray[i] = S;
    if (i - 1 > start)
        qsort_zone(start, i - 1);
    if (j + 1 < end)
        qsort_zone(j + 1, end);
}

static long
extractNonEmptyZoneId()
{
    int zoneId = 0, cnt = 0;
    while(zoneId < NZONES)
    {
        ZoneCtrl* zone = ZoneCtrlArray + zoneId;
        if(zone->pagecnt_dirty > 0)
        {
            ZoneSortArray[cnt] = zoneId;
            cnt++;
        }
        zoneId++;

        if(zone->activate_after_n_cycles > 0)
            zone->activate_after_n_cycles --;
    }
    return cnt;
}

static void
pause_and_score()
{
    /*  For simplicity, searching all the zones of SMR,
        actually it's only needed to search the zones which had been cached.
        But it doesn't matter because of only 200~500K meta data of zones in memory for searching, it's not a big number.
    */
    /* Score all zones. */
    blkcnt_t n = 0;
    ZoneCtrl * myCtrl;
    while(n < NonEmptyZoneCnt)
    {
        myCtrl = ZoneCtrlArray + ZoneSortArray[n];
        myCtrl->score = 0;

        /* score each block of the non-empty zone */
        StrategyDesp_pore * desp;
        blkcnt_t despId = myCtrl->head;
        while(despId >= 0)
        {
            desp = GlobalDespArray + despId;
            long idx2 = CycleID - desp->stamp;
            if(idx2 > 15)
                idx2 = 15;

            myCtrl->score += (0x00000001 << idx2);

            despId = desp->next;
        }
        n++ ;
    }
}


static int
redefineOpenZones()
{
    NonEmptyZoneCnt = extractNonEmptyZoneId();
    if(NonEmptyZoneCnt == 0)
        return 0;
    pause_and_score(); /** ARS (Actually Release Space) */
    qsort_zone(0,NonEmptyZoneCnt-1);

    long max_n_zones = Cycle_Length / (ZONESZ / BLKSZ);
    if(max_n_zones == 0)
        max_n_zones = 1;  // This is for Emulation on small traces, some of their fifo size are lower than a zone size.

    OpenZoneCnt = 0;
    long i = 0;
    while(OpenZoneCnt < max_n_zones && i < NonEmptyZoneCnt)
    {
        ZoneCtrl* zone = ZoneCtrlArray + ZoneSortArray[i];

        /* According to the RULE 2, zones which have already be in PB cannot be choosed into this cycle. */
        if(zone->activate_after_n_cycles == 0)
        {
            zone->activate_after_n_cycles = 2;  // Deactivate the zone for the next 2 cycles.
            OpenZoneSet[OpenZoneCnt] = zone->zoneId;
            OpenZoneCnt++;
        }
        i++;
    }

    /** lookup sort result **/
//    int i;
//    for(i = 0; i<NonEmptyZoneCnt; i++)
//    {
//        printf("%d: score=%f\t\theat=%ld\t\tndirty=%ld\t\tnclean=%ld\n",
//               i,
//               ZoneCtrlArray[ZoneSortArray[i]].score,
//               ZoneCtrlArray[ZoneSortArray[i]].heat,
//               ZoneCtrlArray[ZoneSortArray[i]].pagecnt_dirty,
//               ZoneCtrlArray[ZoneSortArray[i]].pagecnt_clean);
//    }

    return OpenZoneCnt;
}

static int
get_FrozenOpZone_Seq()
{
    int seq = 0;
    blkcnt_t frozenSeq = -1;
    long frozenStamp = CycleID;
    while(seq < OpenZoneCnt)
    {
        ZoneCtrl* ctrl = ZoneCtrlArray + OpenZoneSet[seq];
        if(ctrl->pagecnt_dirty <= 0)
        {
            seq ++;
            continue;
        }

        StrategyDesp_pore* tail = GlobalDespArray + ctrl->tail;
        if(tail->stamp < frozenStamp)
        {
            frozenStamp = tail->stamp;
            frozenSeq = seq;
        }
        seq ++;
    }

    return frozenSeq;   // If return value <= 0, it means 1. here has no dirty block. 2. here has not started the cycle.
}

static void stamp(StrategyDesp_pore * desp)
{
    desp->stamp = CycleID;
}

static int random_pick(float weight1, float weight2, float obey)
{
    //return (weight1 < weight2) ? 1 : 2;
    // let weight as the standard, set as 1,
    float inc_times = (weight2 / weight1) - 1;
    inc_times *= obey;

    float de_point = 1000 * (1 / (2 + inc_times));
//    rand_init();
    int token = rand() % 1000;

    if(token < de_point)
        return 2;
    return 1;
}
