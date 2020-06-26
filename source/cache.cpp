// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE
// _____________________________________________________________________________________________________________________________________________
// CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE...CACHE

#include "cache.h"
#include "block.h"
#include "partition.h"


// construct the cache of the given capacity
Cache::Cache(siz32 cap)
{
    // reserve memory for the blocks that will be cached
    block = new Block[cap];
    // save the given capacity
    capacity = cap;

    // create a temporary cache slot descriptor
    CacheSlot cs;

    // for every cache slot descriptor in the cache
    for( idx32 idx = 0; idx < cap; idx++ )
    {
        // save the slot's current location in the cache
        cs.setLocation(idx);
        // add the descriptor to the free slot pool
        pool[ifree].push(cs);
    }
}

// destruct the cache
// IMPORTANT: the destructor doesn't flush dirty blocks to the partition -- this step must be done before the destructor is called
Cache::~Cache()
{
    // delete the block array and reset the block array pointer to null
    if( block ) { delete[] block; block = nullptr; }

    // reset the cache capacity to zero
    capacity = 0;

    // clear the slot pools
    for( idx32 i = 0; i < poolcnt; i++ )
        pool[i].clear();
}



// read a block from the disk partition, through the cache, into the given buffer
MFS Cache::readFromPart(Partition* part, idx32 blkid, Buffer buffer)
{
    // if the cache has been destroyed, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;
    // if the buffer doesn't exist, return an error code
    if( !buffer ) return MFS_BADARGS;

    // create a variable that will hold the current pool index
    idx32 ipool;
    // create a variable that will hold the location of the slot in its pool
    idx64 slotloc;

    // try to find the cache slot that holds the block with the given block id, if the search is unsuccessful (the block isn't loaded in the cache)
    if( findSlot(blkid, ipool, slotloc) != MFS_OK )
    {
        // if there are no free slots available in the free pool, try to free some slots from the other pools
        if( pool[ifree].size() == 0 ) freeThisManySlots(part, (siz32) std::ceil(capacity * CacheFreePercent));

        // try to load the block with the given id from the disk into the cache, if the load fails return an error code
        if( loadEmptySlotFromPart(part, blkid, ipool, slotloc) != MFS_OK ) return MFS_ERROR;
    }

    // make a reference to the found cache slot
    CacheSlot& slot = pool[ipool].at(slotloc);
    // copy the contents of the block in cache into the given buffer
    block[slot.position].copyToBuffer(buffer);

    // increase the slot access count
    slot.accesscnt++;
    // update the slot position in its pool
    pool[ipool].update(slotloc);

    // return that the operation was successful
    return MFS_OK;
}

// write a block from the given buffer, through the cache, into the disk partition
MFS Cache::writeToPart(Partition* part, idx32 blkid, const Buffer buffer)
{
    // if the cache has been destroyed, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;
    // if the buffer doesn't exist, return an error code
    if( !buffer ) return MFS_BADARGS;

    // create a variable that will hold the current pool index
    idx32 ipool;
    // create a variable that will hold the location of the slot in its pool
    idx64 slotloc;
    // a bool that tells if the found block should be overwritten
    bool overwritten = false;

    // try to find the cache slot that holds the block with the given block id, if the search is unsuccessful (the block isn't loaded in the cache)
    if( findSlot(blkid, ipool, slotloc) != MFS_OK )
    {
        // if there are no free slots available in the free pool, try to free some slots from the other pools
        if( pool[ifree].size() == 0 ) freeThisManySlots(part, (siz32) std::ceil(capacity * CacheFreePercent));

        // try to load the block with the given id from the buffer into the cache, if the load fails return an error code
        if( loadEmptySlotFromBuffer(buffer, blkid, ipool, slotloc) != MFS_OK ) return MFS_ERROR;

        // the block has been loaded in the cache, thus it shouldn't be written to again
        overwritten = true;
    }

    // make a reference to the found cache slot
    CacheSlot& slot = pool[ipool].at(slotloc);
    // if the block hasn't been overwritten, copy the contents of the buffer into the block in cache
    if( !overwritten ) block[slot.position].copyFromBuffer(buffer);

    // increase the slot access count
    slot.accesscnt++;
    // update the slot position in its pool
    pool[ipool].update(slotloc);

    // return that the operation was successful
    return MFS_OK;
}



// get the number of slots in the cache
siz32 Cache::getSlotCount() const { return capacity; }
// get the number of free slots in the cache
siz32 Cache::getFreeCount() const { return (siz32) pool[ifree].size(); }
// get the number of clean slots in the cache
siz32 Cache::getCleanCount() const { return (siz32) pool[iclean].size(); }
// get the number of dirty slots in the cache
siz32 Cache::getDirtyCount() const { return (siz32) pool[idirty].size(); }



// try to flush the requested number of slots in the cache to the disk, return the actual number of flushed slots
// IMPORTANT: also decreases the slot access count by one for every non-free slot in the cache
MFS32 Cache::flushThisManySlots(Partition* part, siz32 count)
{
    // if the cache has been destroyed, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;

    // create a variable that will hold the actual number of flushed slots
    siz32 flushedcnt = 0;

    // flush slots in the dirty pool
    {
        // create a temporary variable that will hold the location of the newly moved slot in its pool
        idx64 nextloc;

        // for every slot in the dirty pool and while the flushed quota hasn't been reached
        for( idx32 slotloc = 0;   slotloc < pool[idirty].size() && flushedcnt < count;   )
        {
            // make a reference to the current cache slot
            CacheSlot& slot = pool[idirty].at(slotloc);

            // if the write to the disk was not successful or the slot move to the clean pool was not successful
            if( part->writeCluster(slot.blockid, block[slot.position]) != MFS_PART_OK || moveSlot(idirty, slotloc, iclean, nextloc) != MFS_OK )
            {
                // skip the current slot and continue from the next slot
                // IMPORTANT: this doesn't behave as the LRU algorithm, but hopefully this won't happen often
                slotloc++;
                // continue from the next slot
                continue;
            }

            // increment the number of flushed slots
            flushedcnt++;
            // continue from the current slot position (since the slot at the current position has been replaced with another not yet flushed slot)
        }
    }

    // decrease the access counts by one in the clean and dirty pool
    {
        // for every slot in the clean pool
        for( idx32 slotloc = 0;   slotloc < pool[iclean].size();   slotloc++ )
        {
            // make a reference to the current cache slot
            CacheSlot& slot = pool[iclean].at(slotloc);

            // decrease the slot access count if it is greater than zero
            if( slot.accesscnt > 0 ) slot.accesscnt--;
        }
        // rebuild the clean pool
        pool[iclean].rebuild();

        // for every slot in the dirty pool
        for( idx32 slotloc = 0;   slotloc < pool[idirty].size();   slotloc++ )
        {
            // make a reference to the current cache slot
            CacheSlot& slot = pool[idirty].at(slotloc);

            // decrease the slot access count if it is greater than zero
            if( slot.accesscnt > 0 ) slot.accesscnt--;
        }
        // rebuild the dirty pool
        pool[idirty].rebuild();
    }

    // return the actual number of slots that were flushed
    return flushedcnt;
}

// try to free the requested number of blocks from the cache, and return the actual number of freed blocks
// dirty blocks are flushed to disk before they are removed from cache (clean blocks are just removed)
MFS32 Cache::freeThisManySlots(Partition* part, siz32 count)
{
    // if the cache has been destroyed, return an error code
    if( !block ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;

    // create a variable that will hold the actual number of freed slots
    siz32 freedcnt = 0;

    // free slots in the clean pool
    {
        // create a temporary variable that will hold the location of the newly moved slot in its pool
        idx64 nextloc;

        // for every slot in the clean pool and while the freed quota hasn't been reached
        for( idx32 slotloc = 0;   slotloc < pool[iclean].size() && freedcnt < count;   )
        {
            // make a reference to the current cache slot
            CacheSlot& slot = pool[iclean].at(slotloc);

            // if the slot move to the free pool was unsuccessful
            if( moveSlot(iclean, slotloc, ifree, nextloc) != MFS_OK )
            {
                // skip the current slot and continue from the next slot
                // IMPORTANT: this doesn't behave as the LRU algorithm, but hopefully this won't happen often
                slotloc++;
                // continue from the next slot
                continue;
            }

            // make a reference to the now moved cache slot
            CacheSlot& moved = pool[ifree].at(nextloc);
            // save a copy of the slot descriptor
            CacheSlot backup = moved;

            // release the slot in the free pool
            moved.release();
            // update the slot in the free pool
            // since the slot has changed, provide the unmodified backup as an argument (in order for the hash map inside the heap to be updated correctly)
            pool[ifree].update(nextloc, &backup);

            // increment the number of flushed slots
            freedcnt++;
            // continue from the current slot position (since the slot at the current position has been replaced with another not yet flushed slot)
        }
    }

    // free slots in the dirty pool
    {
        // create a temporary variable that will hold the location of the newly moved slot in its pool
        idx64 nextloc;

        // for every slot in the dirty pool and while the freed quota hasn't been reached
        for( idx32 slotloc = 0;   slotloc < pool[idirty].size() && freedcnt < count;   )
        {
            // make a reference to the current cache slot
            CacheSlot& slot = pool[idirty].at(slotloc);

            // if the write to the disk was not successful or the slot move to the free pool was not successful
            if( part->writeCluster(slot.blockid, block[slot.position]) != MFS_PART_OK || moveSlot(idirty, slotloc, ifree, nextloc) != MFS_OK )
            {
                // skip the current slot and continue from the next slot
                // IMPORTANT: this doesn't behave as the LRU algorithm, but hopefully this won't happen often
                slotloc++;
                // continue from the next slot
                continue;
            }

            // make a reference to the now moved cache slot
            CacheSlot& moved = pool[ifree].at(nextloc);
            // save a copy of the slot descriptor
            CacheSlot backup = moved;

            // release the slot in the free pool
            moved.release();
            // update the slot in the free pool
            // since the slot has changed, provide the unmodified backup as an argument (in order for the hash map inside the heap to be updated correctly)
            pool[ifree].update(nextloc, &backup);

            // increment the number of flushed slots
            freedcnt++;
            // continue from the current slot position (since the slot at the current position has been replaced with another not yet flushed slot)
        }
    }

    // return the actual number of slots that were freed
    return freedcnt;
}



// load a block with the given id from the given buffer into an empty slot in the cache, return if the operation is successful and the pool and location of the slot in the pool
MFS Cache::loadEmptySlotFromBuffer(Buffer buffer, idx32 blkid, idx32& ipool, idx64& slotloc)
{
    // if the cache has been destroyed, return an error code
    if( !block ) return MFS_ERROR;
    // if the cache doesn't have an empty slot left, return an error code
    if( pool[ifree].size() == 0 ) return MFS_ERROR;
    // if the buffer doesn't exist, return an error code
    if( !buffer ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // copy the given buffer contents into the top free block in the free pool
    block[ pool[ifree].at(0).position ].copyFromBuffer(buffer);

    // move the min slot from the free pool into the dirty pool, get its new location in the dirty pool
    moveSlot(ifree, 0, idirty, slotloc);
    // save that the slot is now in the dirty pool
    ipool = idirty;

    // make a reference to the the free slot that was moved to the dirty pool
    CacheSlot& slot { pool[ipool].at(slotloc) };
    // save a copy of the slot descriptor
    CacheSlot backup = slot;

    // reserve the free slot in the dirty pool (since it is in the dirty pool it is now dirty as well)
    slot.reserve(blkid);
    // update the slot in the dirty pool
    // since the slot has changed, provide the unmodified backup as an argument (in order for the hash map inside the heap to be updated correctly)
    pool[ipool].update(slotloc, &backup);

    // return that the operation was successful
    return MFS_OK;
}

// load a block with the given id from the disk partition into an empty slot in the cache, return if the operation is successful and the pool and location of the slot in the pool
MFS Cache::loadEmptySlotFromPart(Partition* part, idx32 blkid, idx32& ipool, idx64& slotloc)
{
    // if the cache has been destroyed, return an error code
    if( !block ) return MFS_ERROR;
    // if the cache doesn't have an empty slot left, return an error code
    if( pool[ifree].size() == 0 ) return MFS_ERROR;
    // if the partition doesn't exist, return an error code
    if( !part ) return MFS_BADARGS;
    // if the block id is invalid, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // read the block from the partition into the first free block in the free pool, if the read fails return an error code
    if( part->readCluster(blkid, block[ pool[ifree].at(0).position ]) != MFS_PART_OK ) return MFS_ERROR;

    // move the min slot from the free pool into the dirty pool, get its new location in the dirty pool
    moveSlot(ifree, 0, idirty, slotloc);
    // save that the slot is now in the dirty pool
    ipool = idirty;

    // make a reference to the the free slot that was moved to the dirty pool
    CacheSlot& slot { pool[ipool].at(slotloc) };
    // save a copy of the slot descriptor
    CacheSlot backup = slot;

    // reserve the free slot in the dirty pool (since it is in the dirty pool it is now dirty as well)
    slot.reserve(blkid);
    // update the slot in the dirty pool
    // since the slot has changed, provide the unmodified backup as an argument (in order for the hash map inside the heap to be updated correctly)
    pool[ipool].update(slotloc, &backup);

    // return that the operation was successful
    return MFS_OK;
}



// find the slot descriptor that holds the block with the given id in one of the pools, return if the operation was successful and the pool and the location of the slot in the pool
MFS Cache::findSlot(idx32 blkid, idx32& ipool, idx64& slotloc)
{
    // if the given block id doesn't exist, return an error code
    if( blkid == nullblk ) return MFS_BADARGS;

    // create a dummy cache slot -- used for searching for the real cache slot that holds the block with the given id
    CacheSlot slot;
    // set its block id to the given block id
    slot.blockid = blkid;

    // for all the cache slot pools
    for( ipool = 0;   ipool < poolcnt;   ipool++ )
    {
        // try to find the location of the actual slot descriptor in the current pool
        slotloc = pool[ipool].find(slot);
        // if the slot descriptor has been found, break
        if( slotloc != nullidx64 ) break;
    }

    // return if the pool containing the slot descriptor has been found
    return ( ipool < poolcnt ) ? MFS_OK : MFS_NOK;
}

// move the slot from one pool to another, return the location of the slot in the next pool
MFS Cache::moveSlot(idx32 currpool, idx64 currloc, idx32 nextpool, idx64& nextloc)
{
    // if the current pool is invalid, return an error code
    if( currpool >= poolcnt ) return MFS_BADARGS;
    // if the next pool is invalid, return an error code
    if( nextpool >= poolcnt ) return MFS_BADARGS;
    // if the current (slot descriptor) location is invalid, return an error code
    if( currloc >= pool[currpool].size() ) return MFS_BADARGS;

    // if the current pool is the same as the next pool
    if( currpool == nextpool )
    {
        // save the location of the cache slot in the 'next' pool
        nextloc = currloc;
        // return that the operation was successful
        return MFS_OK;
    }

    // create a reference to the cache slot to be moved
    CacheSlot& slot = pool[currpool].at(currloc);

    // insert the cache slot into the next pool, if the operation was not successful return an error code
    if( !pool[nextpool].push(slot) ) return MFS_ERROR;
    // save the location of the cache slot in the next pool
    nextloc = pool[nextpool].find(slot);

    // remove the cache slot from the current pool
    pool[currpool].pop(currloc);

    // return that the operation was successful
    return MFS_OK;
}





