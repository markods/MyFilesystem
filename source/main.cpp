// _____________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...M
// _____________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...M
// _____________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...M

#include <iostream>
#include "!global.h"
#include "partition.h"
#include "block.h"


int main()
{
    // for debugging purposes
    #ifdef DEBUG
        void MFS_SYSCFG_CHECK(); MFS_SYSCFG_CHECK();   // check if the standard variables are of the expected sizes
        void MFS_BLOCKS_CHECK(); MFS_BLOCKS_CHECK();   // check if the cache blocks are of the expected sizes
        void MFS_HEAP_CHECK();   MFS_HEAP_CHECK();     // check if the heap works correctly

        return 0;   // exit debug mode (and also program)
    #endif


    char partprefs[] = "p1.ini";
    Partition partition(partprefs);


    Block bitv_blk;
    partition.readCluster(0, bitv_blk);
    std::cout << bitv_blk.bitv << std::endl;

    Block indx_blk;
    partition.readCluster(1, indx_blk);
    std::cout << indx_blk.indx << std::endl;
}



