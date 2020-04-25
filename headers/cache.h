// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE

#pragma once
#include <mutex>
#include "!global.h"
#include "cacheslot.h"
#include "heap.h"
union Block;
class Partition;


// cache for disk blocks
// all the methods in this class are guarded and reentrant (except for the constructor which is unguarded,
//                       since the object can only be accessed by a single thread during its construction)
class Cache
{
private:
    Heap<CacheSlot> heapmap;   // heap (and hash map) of slot descriptors
    Block* block;              // an array (pool) of blocks (stores blocks the cache slots describe)

    siz32 SlotCount;           // capacity of cache in slots (blocks)
    uns32 FreeSlots;           // number of free slots in cache

    std::recursive_mutex m;    // reentrant mutex used for locking class methods


public:
    // construct the cache of given fixed size
    Cache(siz32 slotcnt);
    // destruct the cache (without flushing dirty blocks!)
    ~Cache();

    // read a block from the disk partition, through the cache, into the given buffer
    MFS readFromPart(Partition* part, idx32 blkid, Block& buffer);
    // write a block from the given buffer, through the cache, into the disk partition
    MFS writeToPart (Partition* part, idx32 blkid, Block& buffer);

private:
    // load a block into cache from given buffer (if there is enough room in the cache), return if successful
    MFS loadSlot(Block& buffer,   idx32 blkid);
    // load a block into cache from the disk partition (if there is enough room in the cache), return if successful
    MFS loadSlot(Partition* part, idx32 blkid);

    // try to flush the requested number of blocks from cache onto the disk, and return the actual number of flushed blocks
    // also decrease the block hitcount by one for every block in the cache
    MFS32 flushSlots(Partition* part, siz32 count);
    // try to free the requested number of blocks from the cache, and return the actual number of freed blocks
    // dirty blocks are flushed to disk before they are removed from cache (clean blocks are just removed)
    MFS32 freeSlots (Partition* part, siz32 count);

    // check if there is at least one free slot in cache, if not then free some slots according to policy and return if there is at least one free slot now
    MFS applyFreePolicy(Partition* part);
};


