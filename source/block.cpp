// _____________________________________________________________________________________________________________________________________________
// BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK
// _____________________________________________________________________________________________________________________________________________
// BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK
// _____________________________________________________________________________________________________________________________________________
// BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK...BLOCK

#include <iostream>
#include <iomanip>
#include "block.h"
using std::ostream;
using std::ios_base;
using std::setw;
using std::hex;
using std::dec;


// ====== empty block header ======
// initialize empty block header
void EmptyBlockHeader::init()
{
    // initialize indexes of the previous and next block in doubly linked list of blocks
    prev = nullblk;
    next = nullblk;
}

// print empty block header to output stream
ostream& operator<<(ostream& os, const EmptyBlockHeader& head)
{
    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('0');

    // write previous and next empty block ids
    os << setw(4*bcw) << hex << head.next << ":ne "
       << setw(4*bcw) << hex << head.prev << ":pr\n";

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}



// ====== data block ======
// initialize data block
void DataBlock::init()
{
    // initialize every byte of the data block to zero
    for( idx32 i = 0; i < Size; i++ )
        byte[i] = (uns8) 0x00;
}

// print data block to output stream
ostream& operator<<(ostream& os, const DataBlock& blk)
{
    constexpr uns32 epl = 16;   // entries per line

    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('0');

    // print data block contents
    uns32 i;
    for( i = 1; i <= DataBlock::Size; i++ )
    {
        os << setw(uns8sz*bcw) << hex << (uns32) blk.byte[i-1];                   // casting byte (uns8) to uns32 for it to be correctly printed
        if     ( i % (epl*lns) == epl ) os << "   #" << dec << i/epl-1 << '\n';   // printing numbered line
        else if( i % epl       == 0   ) os << '\n';                               // printing non-numbered line
        else if( i % 2         == 0   ) os << ' ';                                // grouping two variables (bytes) together
    }
    if( i%epl != 0 ) os << '\n';   // print newline at the end (if it hasn't already been printed)

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}



// ====== index block ======
// initialize index block
void IndexBlock::init()
{
    // initialize every entry in the index block to the invalid block index
    for( idx32 i = 0; i < Size; i++ )
        entry[i] = nullblk;
}

// print index block to output stream
ostream& operator<<(ostream& os, const IndexBlock &blk)
{
    constexpr uns32 epl = 4;   // entries per line

    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('-');

    // print index block entries
    uns32 i;
    for( i = 1; i <= IndexBlock::Size; i++ )
    {
        os << setw(uns32sz*bcw) << hex << blk.entry[i-1];
        if     ( i % (epl*lns) == epl ) os << "   #" << dec << i/4-1 << '\n';   // printing numbered lines
        else if( i % epl       == 0   ) os << '\n';                             // printing non-numbered lines
        else                            os << ' ';                              // spacing between two variables
    }
    if( i%epl != 0 ) os << '\n';

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}



// ====== directory block ======
// initialize directory block
void DirectoryBlock::init()
{
    // free every file descriptor in the directory block
    for( idx32 i = 0; i < Size; i++ )
        filedesc[i].free();
}

// print directory block to output stream
ostream& operator<<(ostream& os, const DirectoryBlock& blk)
{
    // print directory block entries
    for( uns32 i = 0; i < DirectoryBlock::Size; i++ )
    {
        os << blk.filedesc[i];

        if( i % lns == 0 ) os << "   #" << i << '\n';   // printing numbered lines
        else               os << '\n';                  // spacing between two variables
    }

    return os;
}



// ====== bit vector block ======
// initialize directory block
void BitVectorBlock::init()
{
    // reset every bit in the bit vector block
    for( idx32 i = 0; i < BitVectorBlock::Size; i++ )
        bits[i] = (uns8) 0x00;
}

bool BitVectorBlock::isFree (idx32 idx) { return !!( bits[idx/8] & (1u << (7-idx%8)) ); }   // !! - not not, converts the result to bool (values 0 or 1)
bool BitVectorBlock::isTaken(idx32 idx) { return  !( bits[idx/8] & (1u << (7-idx%8)) ); }
void BitVectorBlock::reserve(idx32 idx) { bits[idx/8] &= ~(1u << (7-idx%8)); }
void BitVectorBlock::release(idx32 idx) { bits[idx/8] |=  (1u << (7-idx%8)); }

// print bit vector block to output stream
ostream& operator<<(ostream& os, const BitVectorBlock& blk)
{
    return operator<<( os, *((const DataBlock*) &blk) );
}



// ====== general block -- union of above blocks ======
// copy block contents to given buffer
void Block::copyToBuffer(Buffer buffer) const
{
    for( uns32 i = 0; i < DataBlock::Size; i++ )
        buffer[i] = data.byte[i];
}
// copy given buffer contents to block
void Block::copyFromBuffer(const Buffer buffer)
{
    for( uns32 i = 0; i < DataBlock::Size; i++ )
        data.byte[i] = buffer[i];
}

// cast block as buffer (char*)
Block::operator Buffer() { return (Buffer) (data.byte); }


#ifdef DEBUG
    void MFS_BLOCKS_CHECK()
    {
        static_assert(sizeof(Block)          == ClusterSize,      "size of block is different from cluster size"            );
        static_assert(sizeof(FileDescriptor) == DireBlkEntrySize, "size of directory block entry is different from expected");
    }
#endif
