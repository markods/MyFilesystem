// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE

#pragma once
#include <mutex>
#include <iosfwd>
#include "!global.h"
#include "cacheslot.h"
#include "heap.h"
union Block;
class Partition;


// cache for disk blocks
class Cache
{
private:
    Heap<CacheSlot> heapmap;   // heap (and hash map) of slot descriptors
    Block* block;              // an array (pool) of blocks (stores blocks the cache slots describe)

    siz32 SlotCount;           // capacity of cache in slots (blocks)
    uns32 FreeSlots;           // number of free slots in cache

    std::recursive_mutex m;    // reentrant mutex used for locking class methods


public:
    // construct the cache given its fixed size
    Cache(siz32 slotcnt);
    // destruct the cache
    ~Cache();

    // read a block from the disk partition, through the cache, into the given buffer
    MFS readBlockFrom(Partition* part, idx32 id, Block buffer);
    // write a block to the disk partition from the given buffer
    MFS writeBlockTo(Partition* part, idx32 id, Block buffer);

private:
    // load a block into cache from operating memory
    MFS loadBlock(Block blk, idx32 id);
    // load a block into cache from the disk partition
    MFS loadBlock(Partition* part, idx32 id);

    // flush a given number of blocks from cache onto the disk
    MFS flushBlocks(siz32 count);
    // remove 
    MFS freeSlots(siz32 count);
};


