// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE

#include "cache.h"
#include "block.h"
#include "cacheslot.h"
#include "partition.h"


// construct the cache of given fixed size
Cache::Cache(siz32 slotcnt)
{
    // reserve memory for the blocks in cache
    block = new Block[slotcnt];
    SlotCount = slotcnt;
    FreeSlots = slotcnt;

    // create and fill the slots in the heap's hash map
    for( idx32 i = 0; i < slotcnt; i++ )
    {
        CacheSlot cs{ i };
        heap.push(cs);
    }

}

// destruct the cache
Cache::~Cache()
{
    // clear the heap
    heap.clear();

    // delete the block array and reset the block array pointer to null
    if( block ) { delete[] block; block = nullptr; }

    // reset slotcount and the number of free slots to zero
    SlotCount = 0;
    FreeSlots = 0;
}



// read a block from the disk partition, through the cache, into the given buffer
MFS Cache::readFromPart(Partition* part, idx32 blkid, Block& buffer)
{
    // if the cache has been destroyed in the meantime, return an error code
    if( !block ) return MFS_ERROR;
    // if the given partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // create a temporary cache slot
    CacheSlot temp;
    // set its block id to the given block id
    temp.setBlockId(blkid);
    // try to locate the actual cache slot in the heap that holds the block with the given id
    idx64 hidx = heap.find(temp);

    // if the block isn't already loaded in the cache
    if( hidx == nullidx64 )
    {
        // check if there is at least one free slot after applying the cache freeing policy, if there isn't return an error code
        if( applyFreePolicy(part) != MFS_OK ) return MFS_ERROR;
        // try to load the block with the given id from the disk into the cache, if the load fails return an error code
        if( loadSlot(part, blkid) != MFS_OK ) return MFS_ERROR;
        // find the index of the cache slot that the block was loaded into
        hidx = heap.find(temp);
    }

    // update the cache slot information
    // first make a reference to the cache slot
    CacheSlot& slot = heap.at(hidx);
    // copy the contents of the block (in cache) into the given buffer
    block[slot.getSlotIndex()].copyToBuffer(buffer);         
    // increase the slot hit count (since it was accessed once)
    slot.incHitCount(1);
    // set that the slot has been read from at least once
    slot.setReadFrom();

    // finally update the slot position in the heap
    heap.update(hidx);
    return MFS_OK;
}

// write a block from the given buffer, through the cache, into the disk partition
MFS Cache::writeToPart(Partition* part, idx32 blkid, Block& buffer)
{
    // if the cache has been destroyed in the meantime, return an error code
    if( !block ) return MFS_ERROR;
    // if the given partition doesn't exist, return an error code
    if( !part  ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // create a temporary cache slot
    CacheSlot temp;
    // set its block id to the given block id
    temp.setBlockId(blkid);
    // try to locate the actual cache slot in the heap that holds the block with the given id
    idx64 hidx = heap.find(temp);
    // bool that tells if the block already exists in the cache
    bool block_in_cache = false;

    // if the block isn't already loaded in the cache
    if( hidx == nullidx64 )
    {
        // check if there is at least one free slot after applying the cache freeing policy, if there isn't return an error code
        if( applyFreePolicy(part)   != MFS_OK ) return MFS_ERROR;
        // try to load the block with the given id from the given buffer into the cache, if the load fails return an error code
        if( loadSlot(buffer, blkid) != MFS_OK ) return MFS_ERROR;
        // find the index of the cache slot that the block was loaded into
        hidx = heap.find(temp);
    }
    // otherwise
    else
    {
        // mark that the block has already been loaded in the cache
        block_in_cache = true;
    }

    // update the cache slot information
    // first make a reference to the cache slot
    CacheSlot& slot = heap.at(hidx);

    // overwrite the block if it has already been loaded in the cache
    if( block_in_cache )
    {
        // copy the contents into the block (in cache) from the given buffer
        block[slot.getSlotIndex()].copyFromBuffer(buffer);
    }

    // increase the slot hit count (since it was accessed once)
    slot.incHitCount(1);
    // set that the slot is dirty
    slot.setDirty();

    // finally update the slot position in the heap
    heap.update(hidx);
    return MFS_OK;
}



// get the number of slots in cache
siz32 Cache::getSlotCount() const { return SlotCount; }
// get the number of free slots in cache
siz32 Cache::getFreeSlotCount() const { return FreeSlots; }



// load a block into cache from the given buffer (if there is enough room in the cache), return if successful
MFS Cache::loadSlot(Block& buffer, idx32 blkid)
{
    // if the cache has been destroyed in the meantime, or it doesn't have enough free slots, return an error code
    if( !block || FreeSlots == 0 ) return MFS_ERROR;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // take the least recently used empty slot from the cache (LRU algorithm)
    CacheSlot& slot = heap.top();
    // save the old slot key
    CacheSlot oldkey = slot;

    // copy the given buffer contents into the block that the slot describes
    block[slot.getSlotIndex()].copyFromBuffer(buffer);
    // decrease the number of free slots in the cache
    FreeSlots--;

    // copy the block id into the slot
    slot.setBlockId(blkid);
    // set the slot status as being used (not free)
    slot.rstFree();
    // set the slot status as dirty
    slot.setDirty();
    // set starting slot hit count (give the slot a chance to stay longer in the cache since it's just been filled)
    slot.incHitCount(StartingHitCount);

    // update the slot position in the heap (it is the top (first) element in the heap)
    // since the block id has changed, provide the old slot id as a function argument (in order for the hash map inside the heap to correctly update the entry)
    // this has to be done as the last step, since the slot reference in this function won't point to the correct slot if the slot is moved in the heap (if the heap is updated)
    heap.update(0, &oldkey);

    // return that the operation was successful
    return MFS_OK;
}

// load a block into cache from the disk partition (if there is enough room in the cache), return if successful
MFS Cache::loadSlot(Partition* part, idx32 blkid)
{
    // if the cache has been destroyed in the meantime, or it doesn't have enough free slots, return an error code
    if( !block || FreeSlots == 0 ) return MFS_ERROR;
    // if the given partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // take the least recently used empty slot from the cache (LRU algorithm)
    CacheSlot& slot = heap.top();
    // save the old slot key
    CacheSlot oldkey = slot;

    // copy the given buffer contents into the block that the slot describes
    // if the copy fails, return an error code
    if( part->readCluster(blkid, block[slot.getSlotIndex()]) != MFS_PART_OK ) return MFS_ERROR;
    // decrease the number of free slots in the cache
    FreeSlots--;

    // copy the block id into the slot
    slot.setBlockId(blkid);
    // set the slot status as being used (not free)
    slot.rstFree();
    // set starting slot hit count (give the slot a chance to stay longer in the cache since it's just been filled)
    slot.incHitCount(StartingHitCount);

    // update the slot position in the heap (it is the top (first) element in the heap)
    // since the block id has changed, provide the old slot id as a function argument (in order for the hash map inside the heap to correctly update the entry)
    // this has to be done as the last step, since the slot reference (in this function) won't point to the correct slot if the slot is moved in the heap (if the heap is updated)
    heap.update(0, &oldkey);

    // return that the operation was successful
    return MFS_OK;
}



// try to flush the requested number of blocks from cache onto the disk, and return the actual number of flushed blocks
// also decrease the block hitcount by one for every block in the cache
MFS32 Cache::flushSlots(Partition* part, siz32 count)
{
    // if the cache has been destroyed in the meantime, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part  ) return MFS_BADARGS;

    // flushed counter
    siz32 flushed = 0;
    // for each element in the heap
    for( idx32 hidx = 0; hidx < heap.size(); hidx++ )
    {
        // make a reference to the current cache slot
        CacheSlot& slot = heap.at(hidx);
        // decrease the slot hit count
        slot.incHitCount(-1);

        // if the slot is dirty and the write to the disk was successful
        if( slot.isDirty() && part->writeCluster(slot.getBlockId(), block[slot.getSlotIndex()]) == MFS_PART_OK )
        {
            // reset the slot dirty flag
            slot.rstDirty();
            // increment the flushed counter
            flushed++;
        }
    }

    // finally rebuild the heap
    heap.rebuild();
    // return the actual number of blocks flushed
    return flushed;
}

// try to free the requested number of blocks from the cache, and return the actual number of freed blocks
// dirty blocks are flushed to disk before they are removed from cache (clean blocks are just removed)
MFS32 Cache::freeSlots(Partition* part, siz32 count)
{
    // if the cache has been destroyed in the meantime, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part  ) return MFS_BADARGS;

    // cleared counter (for counting the number of cleared cache slots)
    siz32 cleared = 0;
    // for each element in the heap and while the number of cleared elements is less than requested:
    for( idx32 hidx = 0; hidx < heap.size() && cleared < count; hidx++ )
    {
        // make a reference to the current cache slot
        CacheSlot& slot = heap.at(hidx);

        // if the cache slot is dirty and the write to the disk was successful
        if( slot.isDirty() && part->writeCluster(slot.getBlockId(), block[slot.getSlotIndex()]) == MFS_PART_OK )
        {
            // set that the slot isn't dirty anymore
            slot.rstDirty();
        }

        // if the slot isn't dirty anymore but it isn't free (if the slot is dirty it can't be removed or data loss will occur)
        if( !slot.isDirty() && !slot.isFree() )
        {
            // initialize the cache slot (free, not dirty, not readfrom, hit count is zero, block id is invalid, keep the cache slot index)
            slot.init();

            // increment the cleared counter
            cleared++;
        }
    }

    // if at least one slot has been cleared, rebuild the entire heap
    // (we have to rebuild the entire heap since we don't know the indexes of the slots that have been updated)
    if( cleared != 0 ) heap.rebuild();

    // update the number of free slots in the cache
    FreeSlots += cleared;
    
    // return the actual number of blocks cleared
    return cleared;
}

// check if there is at least one free slot in cache, if not then free some slots according to policy and return if there is at least one free slot now
MFS Cache::applyFreePolicy(Partition* part)
{
    // if the cache has been destroyed in the meantime, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part  ) return MFS_BADARGS;

    // if the cache has at least one free slot, return success
    if( FreeSlots > 0 ) return MFS_OK;

    // try to free some slots
    freeSlots(part, (siz32) std::ceil(SlotCount*CacheFreePercent));

    // return if the cache has at least one free slot now
    return (FreeSlots > 0) ? MFS_OK : MFS_ERROR;
}




