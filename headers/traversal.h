// _____________________________________________________________________________________________________________________________________________
// TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL
// _____________________________________________________________________________________________________________________________________________
// TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL
// _____________________________________________________________________________________________________________________________________________
// TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL...TRAVERSAL

#pragma once
#include "!global.h"


// structure used to represent a traversal position inside the root directory/file
struct Traversal
{
    // blocks:            DIRE/DATA  INDX2      INDX1
    // array indexes:     iBLOCK     iINDX2     iINDX1
    idx32 loc[MaxDepth] { nullblk,   nullblk,   nullblk   };   // locations of the          directory/data block (loc[0]) and index blocks (loc[1], loc[2])
    idx32 ent[MaxDepth] { nullidx32, nullidx32, nullidx32 };   // index of entry inside the directory/data block (ent[0]) and index blocks (ent[1], ent[2])
    siz32 depth { 0 };                                         // number of blocks on the traversal path before reaching useful data (number of used array elements in the above arrays)

    siz32 filesScanned { 0 };   // number of files scanned during traversal
    MFS status { MFS_ERROR };   // status of the traversal

    // initialize the traversal to start from the first entry in the given block, and also set the number of blocks on the traversal path
    void init(idx32 StartBlockLocation, siz32 depth);

    // recalculate the depth of the traversal path using the location array
    siz32 recalcDepth();
};

