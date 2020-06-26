// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.
// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.
// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.

#pragma once
#include <iosfwd>
#include "!global.h"


// a structure that represents a single cache slot in the filesystem cache
// IMPORTANT: the cache slot must have a default constructor, since the heap that will hold cache slots requires it
struct CacheSlot
{
    idx32 position { nullidx32 };   // position of the block in the cache's array of blocks
    idx32 blockid { nullblk };      // partition id of the block that occupies this slot
    uns32 accesscnt { 0 };          // number of times the occupied slot has been accessed ( = 0 for free slots)


    // construct the cache slot given a slot index (or using the default invalid slot index)
    CacheSlot(idx32 slotidx = nullidx32);
    // set the new cache slot location and reset the remaining variables
    void setLocation(idx32 slotidx);

    // reserve the cache slot
    void reserve(idx32 blkid);
    // release the cache slot
    void release();

    // lvalue assignment operator for cache slot (rvalue isn't needed since the class has no special fields)
    CacheSlot& operator=(const CacheSlot& slot) = default;

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
// calls operator() defined in the class
namespace std
{
    template<> struct hash<CacheSlot>
    {
        size_t operator()(const CacheSlot& slot) const { return slot.operator()(slot); }
    };
}

