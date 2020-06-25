#include <iostream>
#include "!global.h"
#include "partition.h"
#include "block.h"
#include "fs.h"
#include "file.h"


int MFS_TEST_03()
{
    std::cout << "==============================< TEST 03 >======" << std::endl;

    BytesCnt bytescnt = 48;
//  char buffer[] = "ovo je probni string koji ce biti upisan u fajl";
    char buffer[] = "proba";


    char partprefs[] = "p3.ini";
    Partition partition { partprefs };

 // Block bitv_blk;
 // partition.readCluster(0, bitv_blk);
 // std::cout << bitv_blk.bitv << std::endl;
 // 
 // Block indx_blk;
 // partition.readCluster(1, indx_blk);
 // std::cout << indx_blk.indx << std::endl;

    // -----------------------------------------

    if( FS::mount(&partition) == MFS_FS_OK )
    {
        if( FS::format(true) == MFS_FS_OK )
//      if( FS::format(false) == MFS_FS_OK )
        {
           char filepath[] = "/fajl1.dat";
           File* f = FS::open(filepath, 'w');
           if( f )
           {
           //  f->write(bytescnt, buffer);
               delete f;
           }
        }

        FS::unmount();
    }

    std::cout << "===============================================" << std::endl;
    return 0;
}
