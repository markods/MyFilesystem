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


// class that caches blocks from the disk in an write-back fashion
// the cache uses the least recently used algorithm (LRU) to determine which slots should be freed when space is low
// IMPORTANT: the destructor doesn't flush dirty blocks to the partition -- this step must be done before the destructor is called
class Cache
{
private:
    static const siz32 poolcnt = 3;    // number of cache slot pools (states)
    static const idx32 ifree  = 0;     // cache slots that are empty (do not contain blocks)
    static const idx32 iclean = 1;     // cache slots that contain blocks that are unmodified (there are no modifications to be saved to the disk)
    static const idx32 idirty = 2;     // cache slots that contain modified blocks (the modifications should be saved to the disk)

private:
    Block* block { nullptr };   // space for holding blocks in the cache (each slot describes one block in this space)
    siz32 capacity { 0 };       // capacity of cache in slots

    Heap<CacheSlot> pool[poolcnt] { };   // pools of cache slot descriptors, each pool corresponds to a single state for the cache slot

public:
    // construct the cache of the given capacity
    Cache(siz32 capacity);
    // destruct the cache
    // IMPORTANT: the destructor doesn't flush dirty blocks to the partition -- this step must be done before the destructor is called
    ~Cache();

    // read a block from the disk partition, through the cache, into the given buffer
    MFS readFromPart(Partition* part, idx32 blkid, Buffer buffer);
    // write a block from the given buffer, through the cache, into the disk partition
    MFS writeToPart(Partition* part, idx32 blkid, const Buffer buffer);

    // get the number of slots in the cache
    siz32 getSlotCount() const;
    // get the number of free slots in the cache
    siz32 getFreeCount() const;
    // get the number of clean slots in the cache
    siz32 getCleanCount() const;
    // get the number of dirty slots in the cache
    siz32 getDirtyCount() const;

    // try to flush the requested number of slots in the cache to the disk, return the actual number of flushed slots
    // IMPORTANT: also decreases the slot access count by one for every non-free slot in the cache
    MFS32 flushThisManySlots(Partition* part, siz32 count);
    // try to free the requested number of blocks from the cache, and return the actual number of freed blocks
    // dirty blocks are flushed to disk before they are removed from cache (clean blocks are just removed)
    MFS32 freeThisManySlots(Partition* part, siz32 count);

private:
    // load a block with the given id from the given buffer into an empty slot in the cache, return if the operation is successful and the pool and location of the slot in the pool
    MFS loadEmptySlotFromBuffer(Buffer buffer, idx32 blkid, idx32& ipool, idx64& slotloc);
    // load a block with the given id from the disk partition into an empty slot in the cache, return if the operation is successful and the pool and location of the slot in the pool
    MFS loadEmptySlotFromPart(Partition* part, idx32 blkid, idx32& ipool, idx64& slotloc);

    // find the slot descriptor that holds the block with the given id in one of the pools, return if the operation was successful and the pool and the location of the slot in the pool
    MFS findSlot(idx32 blkid, idx32& ipool, idx64& slotloc);
    // move the slot from one pool to another, return the location of the slot in the next pool
    MFS moveSlot(idx32 currpool, idx64 currloc, idx32 nextpool, idx64& nextloc);
};


