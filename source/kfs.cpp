// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS
// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS
// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS

#include "kfs.h"
#include "block.h"
#include "partition.h"
#include "kfile.h"
// used for locking a function method from when it is entered to its return statement/exception throw/end of body...
using LockGuard = const std::lock_guard<std::mutex>;


// TODO: napraviti thread koji periodicno zove flush kesa! (koristiti std::thread)
// TOOD: napraviti metode AllocateBlock, FreeBlock



// ====== guarded methods ======
// construct the filesystem
KFS::KFS() {}
// destruct the filesystem
KFS::~KFS()
{
    // lock this function body
    LockGuard lock(m_excl);
}



// ====== unguarded methods ======
// mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
// if there is already a mounted partition, make the thread that wants to mount another partition wait until the old partition is unmounted
MFS KFS::_mount(Partition* partition)
{
    // if the given partition doesn't exist, return an error code
    if( !partition ) return MFS_BADARGS;

    // if there is currently a mounted partition, wait until it is unmounted first
    if( part != nullptr )
        ;   // TODO: napraviti sinhronizaciju !

    // create a block that will be used as a bit vector block
    Block BITV;
    // if the bit vector block can't be read from the new partition, return an error code
    if( partition->readCluster(BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;
    
    // mount the new partition
    part = partition;
    // check if the new partition is formatted
    // (the first two bits of the bit vector block have to be both set if the partition is formatted,
    //  since the first block on the partition is always occupied by the bit vector, and the second by the root directory's first level index block)
    formatted = BITV.bitv.getBit(BitvLocation) && BITV.bitv.getBit(RootIndx1Location);
    // reset the previous file count to an invalid value
    filecnt = nullsiz32;

    // return that the mount was successful
    return MFS_OK;
}

// unmount the partition from the filesystem
// wake up a single thread that waited to mount another partition
MFS KFS::_unmount()
{
    // if there is already no mounted partition present, return that the operation was successful
    if( part == nullptr ) return MFS_OK;

    // free all the slots from the cache that come from this mounted partition (since there is at most one mounted partition at any given time, free every occupied block in the cache)
    cache.freeSlots(part, cache.getSlotCount() - cache.getFreeSlotCount());
    // if all the slots couldn't be freed, return an error code (if we skipped this step, we couldn't flush the cache slots that to the given partition later on)
    if( cache.getFreeSlotCount() != cache.getSlotCount() ) return MFS_ERROR;

    // unmount the partition
    part = nullptr;
    // reset the partition formatted state
    formatted = false;
    // reset the partition file count to an invalid value
    filecnt = nullsiz32;

    // TODO: napraviti sinhronizaciju !
    ;

    // return that the operation was successful
    return MFS_OK;
}



// format the mounted partition
MFS KFS::_format()
{
    // if the partition isn't mounted, return an error code
    if( !part ) return MFS_ERROR;

    // create a temporary bit vector block and a temporary level 1 index block (of the root directory)
    Block BITV, DIRE1;

    // initialize the bit vector on the partition (to all zeros)
    BITV.bitv.init();
    // if the write of the bit vector block to the partition failed, return an error code
    if( part->writeCluster(BitvLocation, BITV ) != MFS_OK ) return MFS_ERROR;

    // initialize the first level directory index on the partition
    // (second level directory indexes don't exist until the first file is created => they do not need initialization)
    DIRE1.dire.init();
    // if the write of the first level directory index block to the partition failed, return an error code
    if( part->writeCluster(RootIndx1Location, DIRE1) != MFS_OK ) return MFS_ERROR;

    // set the first two bits of the bit vector block to 1 (meaning that the first two block on the partition are occupied)
    // this code isn't a part of the bit vector initialization because then the partition could be left in an inconsistent state if the write of the fist level directory index block failed!
    BITV.bitv.setBit(BitvLocation);
    BITV.bitv.setBit(RootIndx1Location);
    // if the write of the bit vector block to the partition failed, return an error code
    if( part->writeCluster(BitvLocation, BITV ) != MFS_OK ) return MFS_ERROR;

    // the partition is now formatted
    formatted = true;
    // return that the operation was successful
    return MFS_OK;
}

// check if the mounted partition is formatted
MFS KFS::_isFormatted() const
{
    // if the partition isn't mounted, return an error code
    if( part == nullptr ) return MFS_ERROR;

    // return if the partition is formatted
    return ( formatted ) ? MFS_OK : MFS_NOK;
}



// ====== block info ======
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
// the used entries in the index block and the occupied file descriptors in the directory block must be contiguous! (and begin from the start of the block)
// during block deallocation the entries/file descriptors are compacted!
// for example:
//  >> index block             >> directory block
//  idx      entry status      idx      file descriptor status
//  =====================      ===============================
//  0        occupied          0        taken
//  …        occupied          …        taken
//  k        occupied          k        taken
//  k+1      not occupied      k+1      not taken
//  …        not occupied      …        not taken
//  last     not occupied      last     not taken



// open a file on the mounted partition with the given absolute file path (e.g. /myfile.cpp) and mode ('r'ead, 'w'rite, 'a'ppend)
// +   read and append fail if the file with the given absolute file path doesn't exist
// +   write will try to open a file before writing to it if the file doesn't exist, if it exists it will truncate it before writing
// if multiple threads try to work with a file, the first one gets access and the others have to wait until the first one closed its file handle
KFile* KFS::_openFile(const char* filepath, char mode)
{
    // if there is no mounted partition, return a null pointer
    if( !part ) return nullptr;
    // if the selected mode isn't recognized, return a null pointer
    if( mode != 'r' || mode != 'w' || mode != 'a' ) return nullptr;
    // if the filepath is missing, or it isn't absolute, return a null pointer
    if( !filepath || filepath[0] != '/' ) return nullptr;
    // if the full filename is invalid, return a null pointer
    if( FileDescriptor::isFullNameValid(&filepath[1]) != MFS_OK ) return nullptr;

    // TODO: dovrsiti
}

// delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
// if multiple threads try to work with a file, the first one gets access and the others have to wait until the first one closed its file handle
MFS KFS::_deleteFile(const char* filepath)
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;
    // if the filepath is missing, or it isn't absolute, return an error code
    if( !filepath || filepath[0] != '/' ) return MFS_ERROR;
    // if the full filename is invalid, return an error code
    if( FileDescriptor::isFullNameValid(&filepath[1]) != MFS_OK ) return MFS_ERROR;

    // TODO: dovrsiti
    MFS32 idxDIRE = _fileExists(filepath);
    if( idxDIRE < 0 ) return MFS_ERROR;
    if( idxDIRE == nullblk ) return MFS_OK;
    

    Block DIRE;


    return MFS_OK;
}



// check if a file exists in the root directory on the mounted partition, if it does return the index of its directory block in the root directory
MFS32 KFS::_fileExists(const char* filepath)
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;
    // if the filepath is missing, or it isn't absolute, return an error code
    if( !filepath || filepath[0] != '/' ) return MFS_ERROR;
    // if the full filename is invalid, return an error code
    if( FileDescriptor::isFullNameValid(&filepath[1]) != MFS_OK ) return MFS_ERROR;


    // create temporary blocks that will hold the root directory's level1 index block, one of its level2 index blocks and one of its directory blocks during the traversal
    Block INDX1, INDX2, DIRE;
    // indexes of the (current) level1 index block, current level2 index block and the current directory block in the partition
    idx32 idxINDX1 = RootIndx1Location;
    idx32 idxINDX2 = nullblk;
    idx32 idxDIRE  = nullblk;

    // current status of the search
    uns32 status = 0;
    constexpr uns32 found = 1<<1;   // status bit -- if the file with the given path is found
    constexpr uns32 error = 1<<2;   // status bit -- if there is an error in the search


    // if the root directory's index1 block couldn't be read, remember that an error occured
    if( cache.readFromPart(part, idxINDX1, INDX1) != MFS_OK )   status |= error;

    // for every index2 block that the root directory's index1 block references
    for( idx32 idx1 = 0; idx1 < IndxBlkSize && !status; idx1++ )
    {
        // if the entry doesn't point to a valid index2 block, return to the previous level of the traversal
        if( ( idxINDX2 = INDX1.indx.entry[idx1] ) == nullblk ) break;
        // if the current index2 block couldn't be read, remember that an error occured
        if( cache.readFromPart(part, idxINDX2, INDX2) != MFS_OK ) status |= error;

        // for every directory block the current index2 block references
        for( idx32 idx2 = 0; idx2 < IndxBlkSize && !status; idx2++ )
        {
            // if the entry doesn't point to a valid directory block, return to the previous level of the traversal
            if( (idxDIRE = INDX2.indx.entry[idx2]) == nullblk ) break;
            // if the current directory block couldn't be read, remember that an error occured
            if( cache.readFromPart(part, idxDIRE, DIRE) != MFS_OK ) status |= error;

            // for every file descriptor in the current directory block
            for( idx32 idx3 = 0; idx3 < IndxBlkSize && !status; idx3++ )
            {
                // if the file descriptor is not taken, return to the previous level of traversal
                if( !DIRE.dire.filedesc[idx3].isTaken() ) break;
                // if the given full file name matches the full file name in the file descriptor, the search is successful
                if( DIRE.dire.filedesc[idx3].cmpFullName(&filepath[1]) == MFS_EQUAL ) status |= found;
            }
        }
    }


    // if an error has occured in the root directory traversal, return an error code
    if( status & error ) return MFS_ERROR;
    // if the file with the given path has been found return the index of its directory block; if not return an invalid index
    return ( status & found ) ? idxDIRE : nullblk;
}

// get the number of files in the root directory on the mounted partition
MFS32 KFS::_getRootFileCount()
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;

    // if the filecnt isn't an invalid (uninitialized) number, return it
    // (it is being managed by the filesystem class whenever files are added and removed)
    if( filecnt != nullsiz32 ) return filecnt;
    // reset the file count to zero (so that it can be incremented properly)
    filecnt = 0;

    // create temporary blocks that will hold the root directory's level1 index block, one of its level2 index blocks and one of its directory blocks during the traversal
    Block INDX1, INDX2, DIRE;
    // indexes of the (current) level1 index block, current level2 index block and the current directory block in the partition
    idx32 idxINDX1 = RootIndx1Location;
    idx32 idxINDX2 = nullblk;
    idx32 idxDIRE  = nullblk;

    // current status of the search
    uns32 status = 0;
    constexpr uns32 error = 1<<1;   // status bit -- if there is an error in the search


    // if the root directory's index1 block couldn't be read, remember that an error occured
    if( cache.readFromPart(part, idxINDX1, INDX1) != MFS_OK )   status |= error;

    // for every index2 block that the root directory's index1 block references
    for( idx32 idx1 = 0; idx1 < IndxBlkSize && !status; idx1++ )
    {
        // if the entry doesn't point to a valid index2 block, return to the previous level of the traversal
        if( (idxINDX2 = INDX1.indx.entry[idx1]) == nullblk ) break;
        // if the current index2 block couldn't be read, remember that an error occured
        if( cache.readFromPart(part, idxINDX2, INDX2) != MFS_OK ) status |= error;

        // for every directory block the current index2 block references
        for( idx32 idx2 = 0; idx2 < IndxBlkSize && !status; idx2++ )
        {
            // if the entry doesn't point to a valid directory block, return to the previous level of the traversal
            if( (idxDIRE = INDX2.indx.entry[idx2]) == nullblk ) break;
            // if the current directory block couldn't be read, remember that an error occured
            if( cache.readFromPart(part, idxDIRE, DIRE) != MFS_OK ) status |= error;

            // for every file descriptor in the current directory block
            for( idx32 idx3 = 0; idx3 < IndxBlkSize && !status; idx3++ )
            {
                // if the file descriptor is not taken, return to the previous level of traversal
                if( !DIRE.dire.filedesc[idx3].isTaken() ) break;
                // if the file descriptor is taken, increase the file count
                filecnt++;
            }
        }
    }


    // if an error has occured in the root directory traversal
    if( status & error )
    {
        // reset the file count to an invalid number
        filecnt = nullsiz32;
        // return an error code
        return MFS_ERROR;
    }

    // return the newly calculated file count
    return filecnt;
}




