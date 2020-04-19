// _____________________________________________________________________________________________________________________________________________
// BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK
// _____________________________________________________________________________________________________________________________________________
// BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK
// _____________________________________________________________________________________________________________________________________________
// BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK

#pragma once
#include <iosfwd>
#include "!global.h"
#include "fd.h"


// !!! this block is currently unused !!!
// the empty block only has a header, which links it to other empty blocks in a doubly linked list
struct EmptyBlockHeader
{
    idx32 next, prev;
    friend std::ostream& operator<<(std::ostream& os, const EmptyBlockHeader& head);
};

// the data block holds N bytes of information
// it doesn't have an inner structure as far as the filesystem is concerned (it could have an inner structure though)
struct DataBlock
{
    uns8 byte[DataBlkSize];
    friend std::ostream& operator<<(std::ostream& os, const DataBlock& blk);
};

// the index block indexes other blocks (data, index or directory blocks)
// it comes in two variants: + level 1 index
//                           + level 2 index
//
// level 1 index   ->   N level 2 indexes    (-> means indexes)
// level 2 index   ->   N data blocks
// 
struct IndexBlock
{
    idx32 entry[IndxBlkSize];
    friend std::ostream& operator<<(std::ostream& os, const IndexBlock& blk);
};

// the directory block contains information about files present in the filesystem
// each directory block entry holds the name of the file, its size and a pointer to the file's index blocks
// the file is structured as shown on the diagram:
//  directory blocks    file blocks
//  ==============      --------------
//  i1*   i2     dir    i1    i2    dat
//  ⬒ →→→ ⬒ →→→ ⬢ →→→ ⬒ →→→ ⬒ →→→ ❒
//       ↘ ⬒   ↘ ⬢   ↘ ⬒   ↘ ⬒   ↘ ❒
//       ↘ ⬒   ↘ ⬢   ↘ ⬒   ↘ ⬒   ↘ ❒
//       ↘ …    ↘ …   ↘ …    ↘ …   ↘ …
// 
// i1* - the partition's directory idx1 block is at a predefined location
// 
struct DirectoryBlock
{
    FileDescriptor entry[DireBlkSize];
    friend std::ostream& operator<<(std::ostream& os, const DirectoryBlock& blk);
};

// the bit vector block contains the partition bit vector
// the bit vector tells which blocks on a partition are free, and which are occupied by meaningful data
struct BitVectorBlock
{
private:
    uns8 bits[BitvBlkSize];

public:
    bool getBit(uns32 idx);
    void setBit(uns32 idx);
    void rstBit(uns32 idx);

    friend std::ostream& operator<<(std::ostream& os, const BitVectorBlock& blk);
};


// the block is a union of the above types of blocks
// it can be treated as any of the existing block types
union Block
{
    // EmptyBlockHeader head;
    DataBlock        data;
    IndexBlock       indx;
    DirectoryBlock   dire;
    BitVectorBlock   bitv;

    void init(uns8 initval);
    void copyToBuffer(Buffer buffer) const;
    void copyFromBuffer(const Buffer buffer);
    operator Buffer();
};


