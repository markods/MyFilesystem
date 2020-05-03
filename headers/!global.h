// _____________________________________________________________________________________________________________________________________________
// GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL
// _____________________________________________________________________________________________________________________________________________
// GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL
// _____________________________________________________________________________________________________________________________________________
// GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL...GLOBAL

#pragma once


// ====== for debugging ======
// if this symbol is defined, then unit testing functions will be defined in various other files
#define DEBUG


// ====== types ======
// explicitly define standard C++ types with known bit sizes
using uns8  = unsigned char;
using uns16 = unsigned short;
using uns32 = unsigned long;
using uns64 = unsigned long long;

using int8  = signed char;
using int16 = signed short;
using int32 = signed long;
using int64 = signed long long;

using flo32 = float;
using flo64 = double;

// additional useful types
using idx32 = uns32;
using idx64 = uns64;

using siz32 = uns32;
using siz64 = uns64;

// define compile-time variables that tell the sizes of standard types in bytes
constexpr auto uns8sz  = 1;
constexpr auto uns16sz = 2;
constexpr auto uns32sz = 4;
constexpr auto uns64sz = 8;

constexpr auto int8sz  = 1;
constexpr auto int16sz = 2;
constexpr auto int32sz = 4;
constexpr auto int64sz = 8;

constexpr auto flo32sz = 4;
constexpr auto flo64sz = 8;

// define minimal and maximal values that the standard types can have
constexpr uns32 uns32max = ~0UL;
constexpr uns64 uns64max = ~0ULL;

// define special values for indexes and sizes
constexpr idx32 nullidx32 = uns32max;
constexpr idx64 nullidx64 = uns64max;
constexpr siz32 nullsiz32 = uns32max;
constexpr siz64 nullsiz64 = uns64max;



// ====== MFS constants ======
// mfs types
using MFS   = int32;                // type for holding only statuses
using MFS32 = int32;                // type for holding a non-negative 32-bit integer or error codes (less than zero)
using MFS64 = int64;                // type for holding a non-negative 64-bit integer or error codes (less than zero)

// mfs status codes
// success codes are > 0, and error codes are <= 0
constexpr int32 MFS_GREATER =  4;   // comparison between A and B returned greater (A>B)
constexpr int32 MFS_EQUAL   =  3;   // comparison between A and B returned equal (A==B)
constexpr int32 MFS_LESS    =  2;   // comparison between A and B returned less (A<B)

constexpr int32 MFS_OK      =  1;   // operation was successful (similar to true)
constexpr int32 MFS_NOK     =  0;   // operation was unsuccessful (similar to false)
constexpr int32 MFS_BADARGS = -1;   // bad arguments were given in function call
constexpr int32 MFS_ERROR   = -2;   // operation failed due to some internal error
// ...                              // user-defined status codes should be >= 10 or <= -10

// constants for pretty printing to otput stream
constexpr uns32 bcw = 2;    // byte character width when output to ostream as hex value (one byte = two hexadecimal digits)
constexpr uns32 lns = 5;    // line number spacing



// ====== types and constants in file "partition.h" ======
using ClusterNo = idx32;
using Buffer    = char*;
constexpr ClusterNo nullblk = uns32max;   // invalid block id
constexpr siz32 ClusterSize = 2048;       // size of cluster on disk
constexpr int32 MFS_PART_OK  = 1;         // partition operation successful
constexpr int32 MFS_PART_ERR = 0;         // partition operation unsuccessful or error



// ====== types and constants in file "fd.h" ======
using FileCnt  = siz32;
using BytesCnt = siz32;
constexpr siz32 FileNameSize = 8+1;   // maximum length of filename in bytes (without extension, '.' and including '\0')
constexpr siz32 FileExtSize  = 3+1;   // maximum length of file extension in bytes (including '\0')
constexpr siz32 FullFileNameSize = FileNameSize-1 + 1 + FileExtSize;   // maximum length of filename + '.' + file extension in bytes (including '\0')



// ====== types and constants in file "block.h" ======
// size of block entries
constexpr siz32 DataBlkEntrySize = 1;    // in bytes
constexpr siz32 IndxBlkEntrySize = 4;    // in bytes
constexpr siz32 DireBlkEntrySize = 32;   // in bytes
constexpr siz32 BitvBlkEntrySize = 1;    // in bytes

// size of blocks in number of entries
constexpr siz32 DataBlkSize = ClusterSize/DataBlkEntrySize;   // in number of entries
constexpr siz32 IndxBlkSize = ClusterSize/IndxBlkEntrySize;   // in number of entries
constexpr siz32 DireBlkSize = ClusterSize/DireBlkEntrySize;   // in number of entries
constexpr siz32 BitvBlkSize = ClusterSize/BitvBlkEntrySize;   // in number of entries

// max file sizes of types Small, Medium and Large in bytes
// (only counting pure data blocks, not counting index 1 block, index 2 block(s) or directory block entry)
constexpr siz32 FileSizeS = 8;                                                 // in bytes (number of data block entries)
constexpr siz32 FileSizeM = FileSizeS +             IndxBlkSize*DataBlkSize;   // in bytes (number of data block entries)
constexpr siz32 FileSizeL = FileSizeM + IndxBlkSize*IndxBlkSize*DataBlkSize;   // in bytes (number of data block entries)



// ====== types and constants in file "cache.h" ======
constexpr int32 StartingHitCount  = 5;      // starting block hit count when the block is 
constexpr flo32 CacheFreePercent  = .05f;   // percent of cache that should be cleared out by the 'free slots'  function
constexpr flo32 CacheFlushPercent = .10f;   // percent of cache that should be flushed out by the 'flush slots' function



// ====== types and constants in file "kfs.h" ======
constexpr siz32 InitialCacheSize  = 100;   // initial filesystem cache size
constexpr idx32 BitvLocation      = 0;     // index of the bit vector block in the partition
constexpr idx32 RootIndx1Location = 1;     // index of the first level index block of the root directory in the partition



