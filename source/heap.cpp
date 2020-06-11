// _____________________________________________________________________________________________________________________________________________
// HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...H
// _____________________________________________________________________________________________________________________________________________
// HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...H
// _____________________________________________________________________________________________________________________________________________
// HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...HEAP...H

#include <iostream>
#include "heap.h"


#ifdef UNIT_TESTING
    void MFS_HEAP_CHECK()
    {
        Heap<int> h;
        for( int i = 100; i >= 0; i-- )
        {
            h.push(i);
            if( i%10 == 0 )
                h.print(std::cout, true);
        }

        for( int i = 100; i >= 0; i-- )
        {
            h.pop();
            if( i%10 == 0 )
                h.print(std::cout, true);
        }
    }
#endif
