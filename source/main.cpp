// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "!global.h"
#include "partition.h"
#include "block.h"


// TODO: staviti u neku klasu
void format(Partition& partition)
{
    Block blk;
    uns32 cid = 0;

    // initializing cluster 0 - bit vector cluster
    blk.init(BitvBlkInitVal);
    blk.bitv.setBit(0);
    blk.bitv.setBit(1);
    partition.writeCluster(++cid, blk);

    // initializing cluster 1 - first level index of directories
    blk.init(IndxBlkInitVal);
    partition.writeCluster(++cid, blk);

    // initializing remaining data/index clusters
    blk.init(DataBlkInitVal);
    for( ; cid < partition.getNumOfClusters(); ++cid )
        partition.writeCluster(cid, blk);
}



int main()
{
    #ifdef DEBUG
        void MFS_SYSCFG_CHECK(); MFS_SYSCFG_CHECK();
        void MFS_BLOCKS_CHECK(); MFS_BLOCKS_CHECK();
        void MFS_HEAP_CHECK();   MFS_HEAP_CHECK();

        return 0;
    #endif





    //char partprefs[] = "p1.ini";
    //Partition partition(partprefs);


    //Block bitv_blk;
    //partition.readCluster(0, bitv_blk);
    //std::cout << bitv_blk.bitv << std::endl;

    //Block indx_blk;
    //partition.readCluster(1, indx_blk);
    //std::cout << indx_blk.indx << std::endl;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
