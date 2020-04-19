// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE

#include <iostream>
#include <iomanip>
#include "cache.h"
#include "block.h"
#include "cacheslot.h"
#include "partition.h"
#include "monitor.h"


// TODO: dodati komentare
Cache::Cache(siz32 slotcnt)
{
    block = new Block[slotcnt];
    CacheSlot cs;
    for( idx32 i = 0; i < slotcnt; i++ )
    {
        cs.setSlotIndex(i);
        heapmap.push(cs);
    }
    SlotCount = slotcnt;
    FreeSlots = slotcnt;
}

Cache::~Cache()
{
    MONITORED(m);
    if( block ) { delete[] block; block = nullptr; }
    SlotCount = 0;
    FreeSlots = 0;
}






MFS Cache::readBlock(idx32 id)
{
    MONITORED(m);
    if( !block ) return MFS_ERROR;
}

MFS Cache::writeBlock(idx32 id)
{
    MONITORED(m);
    if( !block ) return MFS_ERROR;
}



MFS Cache::loadBlock(Block blk, idx32 id)
{
    MONITORED(m);   // the comma (;) is unneded, but it prevents intellisense from inserting an extra tab beneath the macro
    if( !block ) return MFS_ERROR;

    if( FreeSlots == 0 ) freeSlots();
    if( FreeSlots == 0 ) return MFS_ERROR;

    CacheSlot& slot = heapmap.top();
    slot.rstFree();
    slot.incHitCount(1);
    heapmap.update(0);

    FreeSlots--;
    return MFS_OK;
}

MFS Cache::loadBlock(Partition* part, idx32 id)
{
    MONITORED(m);
    if( !block ) return MFS_ERROR;
}



MFS Cache::flushBlocks()
{
    MONITORED(m);
    if( !block ) return MFS_ERROR;
}

MFS Cache::freeSlots()
{
    MONITORED(m);
    if( !block ) return MFS_ERROR;
}







