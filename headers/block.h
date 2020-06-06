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
    idx32 prev, next;

    void init();
    friend std::ostream& operator<<(std::ostream& os, const EmptyBlockHeader& head);
};

// the data block holds N bytes of information
// it doesn't have an inner structure as far as the filesystem is concerned (it could have an inner structure though)
struct DataBlock
{
    uns8 byte[DataBlkSize];
    static const siz32 Size = DataBlkSize;
    
    void init();
    friend std::ostream& operator<<(std::ostream& os, const DataBlock& blk);
};

// the index block indexes other blocks (data, index or directory blocks)
// it comes in two variants:
// +   level 1 index   →   N level 2 indexes    (→ means indexes)
// +   level 2 index   →   N data blocks
// 
// the used entries in the index block must be contiguous! (and begin from the start of the block)
// during block deallocation the entries are compacted!
// for example:
//  >> index block
//  idx      entry status
//  =====================
//  0        occupied
//  …        occupied
//  k        occupied
//  k+1      not occupied
//  …        not occupied
//  last     not occupied
struct IndexBlock
{
    idx32 entry[IndxBlkSize];
    static const siz32 Size = IndxBlkSize;

    void init();
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
// the occupied file descriptors in the directory block must be contiguous! (and begin from the start of the block)
// during block deallocation the file descriptors are compacted!
// for example:
//  >> directory block
//  idx      file descriptor status
//  ===============================
//  0        taken
//  …        taken
//  k        taken
//  k+1      not taken
//  …        not taken
//  last     not taken
struct DirectoryBlock
{
    FileDescriptor filedesc[DireBlkSize];
    static const siz32 Size = DireBlkSize;

    void init();
    friend std::ostream& operator<<(std::ostream& os, const DirectoryBlock& blk);
};

// the bit vector block contains the partition bit vector
// the bit vector tells which blocks on a partition are free, and which are occupied by meaningful data
struct BitVectorBlock
{
private:
    uns8 bits[BitvBlkSize];
    static const siz32 Size = BitvBlkSize;

public:
    void init();

    bool isFree (idx32 idx);
    bool isTaken(idx32 idx);
    void reserve(idx32 idx);
    void release(idx32 idx);

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

    void copyToBuffer(Buffer buffer) const;
    void copyFromBuffer(const Buffer buffer);
    operator Buffer();
};


// a padded block has an empty entry immediately after the block, which should be initialized with the default value for that block type
struct PaddedBlock
{
    Block block;   // the actual block with useful data
    union {
        uns8  byte;
        idx32 entry;
        FileDescriptor filedesc;
        uns8  bits;
    } pad;         // empty entry that should be default initialized
};



