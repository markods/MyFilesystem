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


// default constructor for cache slot (can also be given a slot index)
// the default cache slot has an invalid slot index, an invalid block id, is free, clean, not read from, and its hitcount is zero
// the slot index must have a default, because the heap which will hold these cache slots has to default initialize them!
CacheSlot::CacheSlot(idx32 _slotidx)
{
    // set slot index
    slotidx = _slotidx;
    // FIXME: set block id to be the null block id (maximum possible id) minus the index of the slot in the cache
    blockid = nullblk - (1 + slotidx);

    // reset all slot status flags (since there are unused bits in the status variable)
    status = 0UL;

    // set default slot flags in slot status
    setFree();
    rstDirty();
    rstReadFrom();
    rstHitCount();
}

// initialize the slot with default values for its fields (except for the slot index in the cache, presumably that doesn't change when the slot is reinitialized)
// the reinitialized cache slot has its previous slot index, an invalid block id, is free, clean, not read from, and its hitcount is zero
void CacheSlot::init()
{
    // slot index presumably doesn't change when the cache slot is reinitialized
 // slotidx = unchanged;
    // set block id
    blockid = nullblk - (1 + slotidx);

    // reset all slot status flags (since there are unused bits in the status variable)
    status = 0UL;

    // set default slot flags in slot status
    setFree();
    rstDirty();
    rstReadFrom();
    rstHitCount();
}



// get the block id (describes the position of the block on its partition, currently there is a limit to only one partition in the filesystem)
ClusterNo CacheSlot::getBlockId() const { return blockid; }
// get the slot index in the cache
idx32 CacheSlot::getSlotIndex() const { return slotidx; }

// set the block id (-||-)
void CacheSlot::setBlockId(ClusterNo _blockid) { blockid = _blockid; }
// set the index of this cache slot in its cache
void CacheSlot::setSlotIndex(idx32 _slotidx) { slotidx = _slotidx; }



// return if the slot is free
bool CacheSlot::isFree()       const { return !((status >> notfree_off )    & notfree_mask    ); }
// return if the slot is dirty
bool CacheSlot::isDirty()      const { return !((status >> notdirty_off)    & notdirty_mask   ); }
// return if the slot has been read from in the last iteration
bool CacheSlot::isReadFrom()   const { return !((status >> notreadfrom_off) & notreadfrom_mask); }
// return the slot hitcnt (probably increased multiple times in the last iteration, and definitely decrased once on the )
uns32 CacheSlot::getHitCount() const { return  ((status >> hitcnt_off)      & hitcnt_mask     ); }


// set the slot status as 'free'
void CacheSlot::setFree()     { status &= ~(notfree_mask     << notfree_off    ); }
// set the slot status as 'dirty'
void CacheSlot::setDirty()    { status &= ~(notdirty_mask    << notdirty_off   ); }
// set the slot status as 'readfrom'
void CacheSlot::setReadFrom() { status &= ~(notreadfrom_mask << notreadfrom_off); }
// increase hit count by given delta, return the part of the delta that was applied
int32 CacheSlot::incHitCount(int32 delta)
{
    // save the old hit count
    int32 old_hitcnt = (int32) getHitCount();
    // copy the current hit count
    int32 hitcnt = old_hitcnt + delta;
    // if the hit count is negative, reset it to zero
    if( hitcnt < 0           ) hitcnt = 0;
    // if the hit count is greater than its maximal allowed value, set it to its maximal allowed value
    if( hitcnt > hitcnt_mask ) hitcnt = hitcnt_mask;
    
    // clear the hit count
    status &= ~(hitcnt_mask << hitcnt_off);
    // and set it to the new value
    status |= (uns32) hitcnt;
    
    // return how much the hitcnt changed (the part of the delta that was applied)
    return hitcnt - old_hitcnt;
}


// set the slot status as 'not free'
void CacheSlot::rstFree()     { status |=  (notfree_mask     << notfree_off    ); }
// set the slot status as 'not dirty'
void CacheSlot::rstDirty()    { status |=  (notdirty_mask    << notdirty_off   ); }
// set the slot status as 'not readfrom'
void CacheSlot::rstReadFrom() { status |=  (notreadfrom_mask << notreadfrom_off); }
// reset the slot hit count
void CacheSlot::rstHitCount() { status &= ~(hitcnt_mask      << hitcnt_off     ); }



// used for swapping the cache slots in the heap based on their statuses (so that the cache slot with the best status for being swapped out is always at the top of the heap)
bool operator< (const CacheSlot& slot1, const CacheSlot& slot2) { return slot1.status < slot2.status; }
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
    os << setw(4) << slot.slotidx  << ":slot";

    // if the slot is taken, print its detailed info
    if( !slot.isFree() )
    os << " "
       << hex << setw(4*bcw) << slot.blockid << ":block   " << dec
       <<   (slot.isFree()     ? "free "     : "")
       <<   (slot.isDirty()    ? "dirty "    : "")
       <<   (slot.isReadFrom() ? "readfrom " : "")
       << setw(4) << slot.getHitCount() << ":hitcnt";

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}

