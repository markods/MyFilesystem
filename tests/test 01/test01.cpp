#include <iostream>
#include "!global.h"
#include "partition.h"
#include "block.h"


int MFS_TEST_01()
{
    std::cout << "==============================< TEST 01 >======" << std::endl;

    char partprefs[] = "p1.ini";
    Partition partition { partprefs };

    Block bitv_blk;
    partition.readCluster(0, bitv_blk);
    std::cout << bitv_blk.bitv << std::endl;

    Block indx_blk;
    partition.readCluster(1, indx_blk);
    std::cout << indx_blk.indx << std::endl;

    std::cout << "===============================================" << std::endl;
    return 0;
}
