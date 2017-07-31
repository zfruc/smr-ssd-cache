#include <stdlib.h>
#include "pore.h"
#include "statusDef.h"
#include "losertree4pore.h"
#include "report.h"

static StrategyDesp_pore*   GlobalDespArray;
static ZoneCtrl*            ZoneCtrlArray;

static unsigned long*       ZoneSortArray;      /* The zone ID array sorted by weight(calculated customized). it is used to determine the open zones */
static int                  OpenZoneCnt;        /* It represent the number of open zones and the first number elements in 'ZoneSortArray' is the open zones ID */

static long                 PeriodLenth;        /* The period lenth which defines the times of eviction triggered */
static long                 PeriodProgress;     /* Current times of eviction in a period lenth */
static long                 StampInPeriod;      /* Current io sequenced number in a period lenth, used to distinct the degree of heat among zones */
static int                  IsNewPeriod;


static void add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void move2ArrayHead(StrategyDesp_pore* desp,ZoneCtrl* zoneCtrl);
static long stamp(StrategyDesp_pore* desp);
static void unloadfromZone(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
static void clearDesp(StrategyDesp_pore* desp);
static void hit(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl);
/** PORE **/
static int redefineOpenZones();
static ZoneCtrl* getEvictZone();
static long stamp(StrategyDesp_pore* desp);


static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
InitPORE()
{
    PeriodLenth = NBLOCK_SMR_FIFO;
    StampInPeriod = PeriodProgress = 0;
    IsNewPeriod = 0;
    GlobalDespArray = (StrategyDesp_pore*)malloc(sizeof(StrategyDesp_pore) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl*)malloc(sizeof(ZoneCtrl) * NZONES);

    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);

    int i = 0;
    while(i < NBLOCK_SSD_CACHE){
        StrategyDesp_pore* desp = GlobalDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->heat = 0;
        desp->stamp = 0;
        i++;
    }
    i = 0;
    while(i < NZONES){
        ZoneCtrl* ctrl = ZoneCtrlArray + i;
        ctrl->zoneId = i;
        ctrl->heat = ctrl->pagecnt = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->weight = -1;
        ZoneSortArray[i] = i;
        i++;
    }
    return 0;
}

void*
LogInPoreBuffer(long despId, SSDBufTag tag, unsigned flag)
{
    /* activate the decriptor */
    StrategyDesp_pore* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(tag.offset);
    myDesp->ssd_buf_tag = tag;
    myDesp->flag = flag;

    /* add into chain */
    stamp(myDesp);
    add2ArrayHead(myDesp, myZone);
    myZone->pagecnt++;
}

long
LogOutDesp_pore()
{
    if(PeriodProgress % PeriodLenth == 0)
    {
        // need to rechoose open zones.
        redefineOpenZones();
        PeriodProgress = 0;
    }

    ZoneCtrl*           chosenOpZone = getEvictZone();
    StrategyDesp_pore*  evitedDesp = GlobalDespArray + chosenOpZone->tail;

    unloadfromZone(evitedDesp,chosenOpZone);
    chosenOpZone->pagecnt--;                  /**< Decision indicators */
    chosenOpZone->heat -= evitedDesp->heat;   /**< Decision indicators */
    if((evitedDesp->flag & SSD_BUF_DIRTY) != 0)
        PeriodProgress++;

    clearDesp(evitedDesp);
    return evitedDesp->serial_id;
}

void*
HitPoreBuffer(long despId, unsigned flag)
{
    StrategyDesp_pore* myDesp = GlobalDespArray + despId;
    ZoneCtrl* myZone = ZoneCtrlArray + getZoneNum(myDesp->ssd_buf_tag.offset);

    move2ArrayHead(myDesp,myZone);
    hit(myDesp,myZone);
    stamp(myDesp);
    myDesp->flag = flag;
}

/****************
** Utilities ****
*****************/

static void
hit(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    desp->heat++;
    zoneCtrl->heat++;
}

static void
add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(zoneCtrl->head < 0){
        //empty
        zoneCtrl->head = zoneCtrl->tail = desp->serial_id;
    }else{
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
    if(desp->pre < 0){
        zoneCtrl->head = desp->next;
    }else{
        GlobalDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0){
        zoneCtrl->tail = desp->pre;
    }else{
        GlobalDespArray[desp->next].pre = desp->pre;
    }
    desp->pre = desp->next = -1;
    return 0;
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
}

/* Decision Method */
static volatile void *
qsort_zone(long start, long end)
{
	long		i = start;
	long		j = end;

	ZoneCtrl* curCtrl = ZoneCtrlArray + ZoneSortArray[start];
	while (i < j) {
	    while (ZoneCtrlArray[ZoneSortArray[j]].weight < curCtrl->weight){ j--; }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (ZoneCtrlArray[ZoneSortArray[i]].weight > curCtrl->weight){ i++; }
        ZoneSortArray[j] = ZoneSortArray[i];
	}

	ZoneSortArray[i] = ZoneSortArray[start];
	if (i - 1 > start)
		qsort_zone(start, i - 1);
	if (j + 1 < end)
		qsort_zone(j + 1, end);
}


static volatile void *
pause_and_caculate_weight_sizedivhot()
{
    int n = 0;
    while( n < NZONES ){
        ZoneCtrl* ctrl = ZoneCtrlArray + n;
        ctrl->weight = (ctrl->pagecnt * ctrl->pagecnt)/ctrl->heat * 1000000;
    }
}


static int
redefineOpenZones()
{
    pause_and_caculate_weight_sizedivhot(); /**< Method 1 */
    qsort_zone(0,NZONES-1);

    long n_chooseblk = 0, n = 0;
    while(n<NZONES && n_chooseblk<PeriodLenth)
    {
        n_chooseblk += ZoneCtrlArray[ZoneSortArray[n]].pagecnt;
        n++;
    }
    OpenZoneCnt = n;
    IsNewPeriod = 1;
    return 0;
}

static ZoneCtrl*
getEvictZone()
{
    static void* passport = NULL;
    static int winnerZoneSortId;

    long winnerDespId;
    ZoneCtrl* winnerOz;

    if(IsNewPeriod){
        // Go into the new period, re-creating the loser tree.
        LoserTree_Destory(passport); // to free old tree space.
        StrategyDesp_pore* openZoneTailBlks[OpenZoneCnt];
        int i = 0;
        while(i < OpenZoneCnt){
            ZoneCtrl* oz = ZoneCtrlArray + ZoneSortArray[i];
            openZoneTailBlks[i] = GlobalDespArray + oz->tail;
        }
        if(LoserTree_Create(OpenZoneCnt, openZoneTailBlks, &passport, &winnerZoneSortId, &winnerDespId) < 0)
            error("Create LoserTree Failure.");
        IsNewPeriod = 0;
    }
    else{
        do{
           winnerOz = ZoneCtrlArray + ZoneSortArray[winnerZoneSortId];
           StrategyDesp_pore* candidateDesp = GlobalDespArray + winnerOz->tail;
           LoserTree_GetWinner(passport, candidateDesp, &winnerZoneSortId, &winnerDespId);
           winnerOz = ZoneCtrlArray + ZoneSortArray[winnerZoneSortId];
        }while(winnerOz->tail != winnerDespId);
    }
    return winnerOz;
}

static long
stamp(StrategyDesp_pore* desp)
{
    desp->stamp = ++StampInPeriod;
    return StampInPeriod;
}

