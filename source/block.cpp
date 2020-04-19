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



// print data block to output stream
ostream& operator<<(ostream& os, const DataBlock& blk)
{
    constexpr uns32 epl = 16;   // entries per line

    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('0');

    // print data block contents
    uns32 i;
    for( i = 1; i <= DataBlkSize; i++ )
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



// print index block to output stream
ostream& operator<<(ostream& os, const IndexBlock &blk)
{
    constexpr uns32 epl = 4;   // entries per line

    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('-');

    // print index block entries
    uns32 i;
    for( i = 1; i <= IndxBlkSize; i++ )
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



// print directory block to output stream
ostream& operator<<(ostream& os, const DirectoryBlock& blk)
{
    // print directory block entries
    for( uns32 i = 0; i < DireBlkSize; i++ )
    {
        os << blk.entry[i];

        if( i % lns == 0 ) os << "   #" << i << '\n';   // printing numbered lines
        else               os << '\n';                  // spacing between two variables
    }

    return os;
}



// bit vector block
bool BitVectorBlock::getBit(idx32 idx) { return !!(bits[idx/8] & (1u << (7-idx%8))); }   // !! - not not, converts the result to bool (values 0 or 1)
void BitVectorBlock::setBit(idx32 idx) { bits[idx/8] |=  (1u << (7-idx%8)); }
void BitVectorBlock::rstBit(idx32 idx) { bits[idx/8] &= ~(1u << (7-idx%8)); }

// print bit vector block to output stream
ostream& operator<<(ostream& os, const BitVectorBlock& blk)
{
    return operator<<( os, *((const DataBlock*) &blk) );
}



// general block -- union of above blocks
// initialize general block with given value
void Block::init(uns8 initval)
{
    for( uns32 i = 0; i < DataBlkSize; i++ )
        data.byte[i] = initval;
}

// copy block contents to given buffer
void Block::copyToBuffer(Buffer buffer) const
{
    for( uns32 i = 0; i < DataBlkSize; i++ )
        buffer[i] = data.byte[i];
}
// copy given buffer contents to block
void Block::copyFromBuffer(const Buffer buffer)
{
    for( uns32 i = 0; i < DataBlkSize; i++ )
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
