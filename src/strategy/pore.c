#include "pore.h"

static StrategyDesp_pore*   StrategyDespArray;
static ZoneCtrl*            ZoneCtrlArray;

static unsigned long*       ZoneSortArray;
static int                  OpenZoneCnt = 0;

/* Static Values */
static long                 PeriodLenth;
static long                 StampInPeriod;

static long stamp(StrategyDesp_pore* desp);

static volatile unsigned long
getZoneNum(size_t offset)
{
    return offset / ZONESZ;
}

/* Process Function */
int
initPORE()
{
    StampInPeriod = 0;
    PeriodLenth = NBLOCK_SMR_FIFO;
    StrategyDespArray = (StrategyDesp_pore*)malloc(sizeof(StrategyDesp_pore) * NBLOCK_SSD_CACHE);
    ZoneCtrlArray = (ZoneCtrl*)malloc(sizeof(ZoneCtrl) * NZONES);

    ZoneSortArray = (unsigned long*)malloc(sizeof(unsigned long) * NZONES);

    int i = 0;
    while(i<NBLOCK_SSD_CACHE){
        StrategyDesp_pore* desp = StrategyDespArray + i;
        desp->serial_id = i;
        desp->ssd_buf_tag.offset = -1;
        desp->next = desp->pre = -1;
        desp->hitcnt = 0;
        desp->stamp = 0;
        i++;
    }
    i = 0;
    while(i<NZONES){
        ZoneCtrl* ctrl = ZoneCtrlArray + i;
        ctrl->zoneId = i;
        ctrl->hitcnt = ctrl->pagecnt = 0;
        ctrl->head = ctrl->tail = -1;
        ctrl->weight = -1;
        ZoneSortArray[i] = i;
        i++;
    }
}

void*
assignPOREBuffer(long despId, size_t offset)
{
    /* validate the decriptor */
    StrategyDesp_pore* myDesp = StrategyDespArray + despId;
    myDesp->hitcnt++;
    myDesp->ssd_buf_tag.offset = offset;

}

long
UnloadDesp_pore()
{
    StrategyDesp_pore* desp = StrategyDespArray + despId;
    ZoneCtrl* zoneCtrl = ZoneCtrlArray + getZoneNum(desp->ssd_buf_tag.offset);;

    zoneCtrl->pagecnt--;
    zoneCtrl->hitcnt -= desp->hitcnt;   /**< Decision indicators */

    unloadfromArray(desp,zoneCtrl);
    clearDesp(desp);

    return desp->serial_id;
}


/****************
** Utilities ****
*****************/
static int
add2ArrayHead(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(zoneCtrl->head < 0){
        //empty
        zoneCtrl->head = zoneCtrl->tail = despId;
    }else{
        //unempty
        StrategyDesp_pore* headDesp = StrategyDespArray + zoneCtrl->head;
        desp->pre = -1;
        desp->next = zoneCtrl->head;
        headDesp->pre = desp->serial_id;
        zoneCtrl->head = despId;
    }
}

static int
unloadfromArray(StrategyDesp_pore* desp, ZoneCtrl* zoneCtrl)
{
    if(desp->pre < 0){
        ctrl->head = desp->next;
    }else{
        StrategyDespArray[desp->pre].next = desp->next;
    }

    if(desp->next < 0){
        ctrl->tail = desp->pre;
    }else{
        StrategyDespArray[desp->next].pre = desp->pre;
    }
    return 0;
}

static int
move2ArrayHead(StrategyDesp_pore* desp,ZoneCtrl* zoneCtrl)
{
    unloadfromArray(desp, zoneCtrl);
    add2ArrayHead(desp, zoneCtrl);
    return 0;
}

static void
clearDesp(StrategyDesp_pore* desp)
{
    desp->ssd_buf_tag.offset = -1;
    desp->next = desp->pre = -1;
    desp->hitcnt = 0;
    desp->stamp = 0;
}
static int
periodReset()
{

}

/* Decision Method */
static volatile void *
qsort_zone(long start, long end)
{
	long		i = start;
	long		j = end;

	ZoneCtrl* curCtrl = ZoneCtrlArray + ZoneSortArray[start];
	while (i < j) {
	    while (ZoneCtrlArray[ZoneSortArray[j]].weight < curCtrl.weight){ j--; }
        ZoneSortArray[i] = ZoneSortArray[j];

        while (ZoneCtrlArray[ZoneSortArray[i]].weight > curCtrl.weight){ i++; }
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
        ctrl->weight = (ctrl->pagecnt ^ 2)/ctrl->hitcnt * 1000000;
    }
}

static void
sort_and_count

static int
redefineOpenZones()
{
    pause_and_caculate_weight_hotdivsize(); /**< Method 1 */
    qsort_zone();

    long n_chooseblk = 0, n = 0;
    while(n<NZONES && n_chooseblk<PeriodLenth)
    {
        n_chooseblk += ZoneCtrlArray[ZoneSortArray[n]].pagecnt;
        n++;
    }
    OpenZoneCnt = n;
}

static long
stamp(StrategyDesp_pore* desp)
{
    desp->stamp = ++_StampInPeriod;
    return _StampInPeriod;
}

long
extractZoneIdByFrozenBlk(unsigned long* openZonesCollect, unsigned long openZoneCnt)
{
    unsigned long frozenZoneId = openZonesCollect[0];
    StrategyDesp_pore* frozenDesp = StrategyDespArray + ZoneCtrlArray[frozenZoneId].tail;

    int n = 1;
    while( n < openZoneCnt ){
        unsigned long curZoneId = openZonesCollect[n];
        StrategyDesp_pore* curDesp = StrategyDespArray + ZoneCtrlArray[curZoneId].tail;
        if(curDesp->stamp < frozenDesp->stamp){
            frozenZoneId = curZoneId;
            frozenDesp = curDesp;
        }
        n++;
    }
    return frozenZoneId;
}
