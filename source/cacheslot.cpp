// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.
// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.
// _____________________________________________________________________________________________________________________________________________
// CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT...CACHE.SLOT.

#include <iostream>
#include <iomanip>
#include "cacheslot.h"
using std::ostream;
using std::ios_base;
using std::setw;
using std::hex;
using std::dec;


// construct the cache slot given a slot index (or using the default invalid slot index)
CacheSlot::CacheSlot(idx32 slotidx)
{
    // save the given slot location
    position = slotidx;
    // initialize the rest of the cache slot variables
    release();
}
    
// set the new cache slot location and reset the remaining variables
void CacheSlot::setLocation(idx32 slotidx)
{
    // save the given slot location
    position = slotidx;
    // initialize the rest of the cache slot variables
    release();
}



// reserve the cache slot
void CacheSlot::reserve(idx32 blkid)
{
    // IMPORTANT: the slot index doesn't change when the cache slot is reserved
    ;
    // set the block id
    blockid = blkid;
    // set the number of times the slot has been accessed to the default value
    accesscnt = StartingAccessCount;
}

// release the cache slot
void CacheSlot::release()
{
    // IMPORTANT: the slot index doesn't change when the cache slot is released
    ;
    // reset the block id to an invalid value different from null block (a very large value that is very likely unused)
    // this artificial value has to be used, since all the slots in a heap have to have a unique block id!
    // IMPORTANT: if the partition from which the blocks are read and written to has about 4G addressible blocks, this approximation doesn't work!
    blockid = nullblk - (1 + position);
    // reset the number of times the slot has been accessed
    accesscnt = 0;
}



// used for swapping the cache slots in the heap based on their statuses (so that the cache slot with the best status for being swapped out is always at the top of the heap)
bool operator< (const CacheSlot& slot1, const CacheSlot& slot2) { return slot1.accesscnt < slot2.accesscnt; }
// used for comparing two cache slots with the same hash based on their block ids
bool operator==(const CacheSlot& slot1, const CacheSlot& slot2) { return slot1.blockid == slot2.blockid; }
// used for calculating the hash for the given cache slot in the hash map
// find an entry in the hash map to put the given slot in, based on the block id of the block occupying the slot
size_t CacheSlot::operator()(const CacheSlot& slot) const { return std::hash<ClusterNo>()(slot.blockid); }



// print cache slot to output stream
std::ostream& operator<<(std::ostream& os, const CacheSlot& slot)
{
    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('0');

    // write cache slot info to output stream
    os << setw(4) << slot.position  << ":slot "
       << hex << setw(4*bcw) << slot.blockid << ":block   " << dec
       << setw(4) << slot.accesscnt << ":accesscnt";

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}

