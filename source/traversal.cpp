// _____________________________________________________________________________________________________________________________________________
// TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL
// _____________________________________________________________________________________________________________________________________________
// TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL
// _____________________________________________________________________________________________________________________________________________
// TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL

#pragma once
#include "traversal.h"


// initialize the traversal to start from the first entry in the given block, and also set the number of blocks on the traversal path
void Traversal::init(idx32 StartBlkLocation, siz32 depth)
{
    // initialize the locations and entry indexes to all invalid values
    for( idx32 i = 0; i < MaxDepth; i++ )
    {
        loc[i] = nullblk;
        ent[i] = nullidx32;
    }

    // set the starting block location, and set its entry index to point to the the first entry in the block
    loc[depth] = StartBlkLocation;
    ent[depth] = 0;

    // set the depth to the given value
    this->depth = depth;

    // reset the number of files scanned during the traversal
    filesScanned = 0;

    // reset the traversal status
    status = MFS_ERROR;
}

// recalculate the depth of the traversal path using the location array
siz32 Traversal::recalcDepth()
{
    // reset the traversal path depth
    depth = 0;

    // depth calculation examples:
    //   + not nullblk
    //   - nullblk
    // ====================
    // loc array      depth
    // [ -  -  - ]    0
    // [ +  -  - ]    1
    // [ -  +  - ]    2
    // [ +  +  - ]    2
    // [ -  -  + ]    3
    // [ -  +  + ]    3
    // [ +  -  + ]    impossible (the array of + must be contiguous if the traversal is valid)
    // [ +  +  + ]    3


    // for all the location entries in the array
    for( idx32 idx = 0; idx < MaxDepth; idx++ )
    {
        // if the current location points to a valid block, set its depth as the depth of the traversal (+1 since depth 0 means that the traversal path is empty -- contains no blocks)
        if( loc[idx] != nullblk ) depth = idx + 1;
    }

    // return the traversal path depth
    return depth;
}

