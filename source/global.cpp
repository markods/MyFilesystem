#include "!global.h"

#ifdef DEBUG
    // check if system configuration is different than expected
    void MFS_SYSCFG_CHECK()
    {
        static_assert(sizeof(uns8)  == uns8sz, "sizeof(uns8) is not 1B");
        static_assert(sizeof(uns16) == uns16sz, "sizeof(uns16) is not 2B");
        static_assert(sizeof(uns32) == uns32sz, "sizeof(uns32) is not 4B");
        static_assert(sizeof(uns64) == uns64sz, "sizeof(uns64) is not 8B");

        static_assert(sizeof(int8)  == int8sz, "sizeof(int8) is not 1B");
        static_assert(sizeof(int16) == int16sz, "sizeof(int16) is not 2B");
        static_assert(sizeof(int32) == int32sz, "sizeof(int32) is not 4B");
        static_assert(sizeof(int64) == int64sz, "sizeof(int64) is not 8B");

        static_assert(sizeof(flo32) == flo32sz, "sizeof(flo32) is not 4B");
        static_assert(sizeof(flo64) == flo64sz, "sizeof(flo64) is not 8B");
    }
#endif
