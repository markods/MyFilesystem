// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.
// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.
// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.

#pragma once
#include <iosfwd>
#include "!global.h"


// describes a single cache slot
class CacheSlot
{
private:
    static const uns32 notfree_mask     = 1UL;
    static const uns32 notdirty_mask    = 1UL;
    static const uns32 notreadfrom_mask = 1UL;
    static const uns32 hitcnt_mask      = (1UL<<24)-1;

    static const uns32 notfree_off      = 31UL;
    static const uns32 notdirty_off     = 30UL;
    static const uns32 notreadfrom_off  = 29UL;
    static const uns32 hitcnt_off       =  0UL;

private:
    ClusterNo blockid;   // id of the block (on its partition) that occupies this slot
    idx32 slotidx;       // index of the slot in the cache
    uns32 status;        // contains the following flags and counters -- notfree:1, notdirty:1, notreadfrom:1, hitcnt:24


public:
    // default constructor for cache slot (can also be given a slot index)
    // the default cache slot has a invalid block id, an invalid slot index, is free, clean, not read from, and its hitcount is zero
    CacheSlot(idx32 slotidx = nullidx32);

    ClusterNo getBlockId() const;          // get the block id (describes the position of the block on its partition, currently there is a limit to only one partition in the filesystem)
    idx32 getSlotIndex() const;            // get the slot index in the cache
    void  setBlockId(ClusterNo blockid);   // set the block id (-||-)
    void  setSlotIndex(idx32 slotidx);     // set the index of this cache slot in its cache

    bool isFree()       const;   // return if the slot is free
    bool isDirty()      const;   // return if the slot is dirty
    bool isReadFrom()   const;   // return if the slot has been read from in the last iteration
    uns32 getHitCount() const;   // return the slot hitcnt (probably increased multiple times in the last iteration, and definitely decrased once on the )

    void setFree();                   // set the slot status as 'free'
    void setDirty();                  // set the slot status as 'dirty'
    void setReadFrom();               // set the slot status as 'readfrom'
    int32 incHitCount(int32 delta);   // increase hit count by given delta, return the part of the delta that was applied

    void rstFree();       // set the slot status as 'not free'
    void rstDirty();      // set the slot status as 'not dirty'
    void rstReadFrom();   // set the slot status as 'not readfrom'
    void rstHitCount();   // reset the slot hit count

    // rvalue assignment operator for cache slot (lvalue isn't needed since the class has no special fields)
    CacheSlot& operator=(const CacheSlot& slot);

    // used for swapping the cache slots in the heap based on their statuses (so that the cache slot with the best status for being swapped out is always at the top of the heap)
    friend bool operator< (const CacheSlot& slot1, const CacheSlot& slot2);
    // used for comparing two cache slots with the same hash based on their block ids
    friend bool operator==(const CacheSlot& slot1, const CacheSlot& slot2);
    // used for calculating the hash for the given cache slot in the hash map
    // find an entry in the hash map to put the given slot in, based on the block id of the block occupying the slot
    size_t operator()(const CacheSlot& slot) const;

    // print cache slot to output stream
    friend std::ostream& operator<<(std::ostream& os, const CacheSlot& slot);
};


// template specialization of std::hash<T> for class cache slot
// calls operator()() defined in the class
namespace std
{
    template<> struct hash<CacheSlot>
    {
        size_t operator()(const CacheSlot& slot) const { return slot.operator()(slot); }
    };
}

