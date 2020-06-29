// _____________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...M
// _____________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...M
// _____________________________________________________________________________________________________________________________________________
// MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...MAIN...M

#include "!global.h"


int main()
{
    // if the debugging is turned on
    #ifdef UNIT_TESTING
     // void MFS_SYSCFG_CHECK(); MFS_SYSCFG_CHECK();   // check if the standard variables are of the expected sizes
     // void MFS_BLOCKS_CHECK(); MFS_BLOCKS_CHECK();   // check if the cache blocks are of the expected sizes
     // void MFS_HEAP_CHECK();   MFS_HEAP_CHECK();     // check if the heap works correctly
    #endif
    #ifdef FUNC_TESTING
     // int MFS_TEST_01(); MFS_TEST_01();   // run the test 01
        int MFS_TEST_02(); MFS_TEST_02();   // run the test 02
     // int MFS_TEST_03(); MFS_TEST_03();   // run the test 03
#endif
    #if defined(UNIT_TESTING) || defined(FUNC_TESTING)
        return 0;   // exit the program after testing
    #endif

    // otherwise
 // int usermain(); return usermain();   // call the user-defined main function
}



