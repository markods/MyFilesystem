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






// _____________________________________________________________________________________________________________________________________________
// THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLI
// _____________________________________________________________________________________________________________________________________________
// THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLI

// construct the filesystem
KFS::KFS()
{
    // initialize the event mutexes to locked state (so that the threads trying to access them block)
    mutex_part_unmounted  .lock();
    mutex_all_files_closed.lock();
}

// destruct the filesystem
KFS::~KFS()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // increase the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the all files closed event
        mutex_all_files_closed.lock();

        // the thread only reaches this point when some other thread unmounted the partition
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt--;
    }

    // unconditionally unmount the given partition
    MFS status = unmount_uc();

    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // TODO: sacekati sve niti koje cekaju na nekom event-u ovde -- koristiti up_for_destruction bool u svakoj metodi koja treba da bude probudjena
    //       ne bi trebalo da se proveravaju open files!
    //       prekinuti nit koja radi flush! (unutar nje same), uraditi join sa njom u destruktoru!
    //       postaviti fleg koji kaze da se radi destroy?

    // TODO: napraviti thread koji periodicno zove flush kesa! (koristiti std::thread)
    //       poslati all files closed event unutar file close funkcije

}



// wait until there is no mounted partition
// mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
MFS KFS::mount(Partition* partition)
{
    // if the given partition doesn't exist, return an error code
    if( !partition ) return MFS_BADARGS;

    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // loop until the blocking condition is false
    // if there is already a mounted partition, make this thread wait until it is unmounted
    while( partition != nullptr )
    {
        // increase the number of threads waiting for the unmount event
        mutex_part_unmounted_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the partition unmount event
        mutex_part_unmounted.lock();

        // the thread only reaches this point when some other thread unmounted the partition
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for unmount event
        mutex_part_unmounted_cnt--;
    }

    // unconditionally mount the given partition
    MFS status = mount_uc(partition);
    
    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// wait until all the open files on the partition are closed
// unmount the partition from the filesystem
// wake up a single thread that waited to mount another partition
MFS KFS::unmount()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // increase the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the all files closed event
        mutex_all_files_closed.lock();

        // the thread only reaches this point when some other thread unmounted the partition
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt--;
    }

    // unconditionally unmount the given partition
    MFS status = unmount_uc();

    // if there are threads waiting for the unmount event, unblock one of them
    if( mutex_part_unmounted_cnt > 0 )
    {
        // send an unmount event
        mutex_part_unmounted.unlock();
    }
    else
    {
        // release exclusive access
        mutex_excl.unlock();
    }

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// wait until all the open files on the partition are closed
// format the mounted partition, if there is no mounted partition return an error
MFS KFS::format()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // increase the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the all files closed event
        mutex_all_files_closed.lock();

        // the thread only reaches this point when some other thread unmounted the partition
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt--;
    }

    // unconditionally format the given partition
    MFS status = format_uc();

    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}



// check if the mounted partition is formatted
MFS KFS::isFormatted()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // unconditionally check if the partition is formatted
    MFS status = isFormatted_uc();

    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// get the number of files in the root directory on the mounted partition
MFS32 KFS::getRootFileCount()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // unconditionally check if the file exists
    MFS32 status = getRootFileCount_uc();

    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// check if a file exists in the root directory on the mounted partition, if it does return the index of the directory block containing its file descriptor
MFS KFS::fileExists(const char* filepath)
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;
    // if the filepath is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return MFS_ERROR;

    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // unconditionally check if the file exists (the traversal position is not needed)
    Traversal pos;
    MFS status = fileExists_uc(filepath, pos);

    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}



// wait until no one uses the file with the given filepath
// open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, 'w'rite, 'a'ppend)
// +   read and append fail if the file with the given full path doesn't exist
// +   write will try to open a file before writing to it if the file doesn't exist
KFileHandle KFS::openFile(const char* filepath, char mode)
{
    // TODO: napraviti
}

// close a file with the given full file path (e.g. /myfile.cpp)
// wake up a single thread that waited to open the now closed file
MFS KFS::closeFile(KFileHandle handle)
{
    // TODO: napraviti
}

// delete a file on the mounted partition given the fill file path (e.g. /myfile.cpp)
// the delete will succeed only if the file is not being used by a thread (isn't open)
MFS KFS::deleteFile(const char* filepath)
{
    // TODO: napraviti
}



// read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS32 KFS::readFromFile(KFileHandle handle, idx32 pos, siz32 count, Buffer buffer)
{
    // TODO: napraviti
}

// write the requested number of bytes from the buffer into the file starting from the given position
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS KFS::writeToFile(KFileHandle handle, idx32 pos, siz32 count, Buffer buffer)
{
    // TODO: napraviti
}

// throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem)
MFS KFS::truncateFile(KFileHandle handle, idx32 pos)
{
    // TODO: napraviti
}






// _____________________________________________________________________________________________________________________________________________
// THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS
// _____________________________________________________________________________________________________________________________________________
// THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS

// mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
MFS KFS::mount_uc(Partition* partition)
{
    // if the given partition doesn't exist, return an error code
    if( !partition ) return MFS_BADARGS;

    // create a block that will be used as a bit vector block
    Block BITV;
    // if the bit vector block can't be read from the new partition, return an error code
    if( cache.readFromPart(partition, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;
    
    // mount the new partition
    part = partition;
    // check if the new partition is formatted
    // (the first two bits of the bit vector block have to be both set if the partition is formatted,
    //  since the first block on the partition is always occupied by the bit vector, and the second by the root directory's first level index block)
    formatted = BITV.bitv.isTaken(BitvLocation) && BITV.bitv.isTaken(RootIndx1Location);
    // reset the previous file count to an invalid value
    filecnt = nullsiz32;

    // return that the mount was successful
    return MFS_OK;
}

// unmount the partition from the filesystem
MFS KFS::unmount_uc()
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

    // return that the operation was successful
    return MFS_OK;
}

// format the mounted partition, if there is no mounted partition return an error
MFS KFS::format_uc()
{
    // if the partition isn't mounted, return an error code
    if( !part ) return MFS_ERROR;

    // create a temporary bit vector block and a temporary level 1 index block (of the root directory)
    Block BITV, DIRE1;

    // initialize the bit vector on the partition (to all zeros)
    BITV.bitv.init();
    // if the write of the bit vector block to the partition failed, return an error code
    if( cache.writeToPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;

    // initialize the first level directory index on the partition
    // (second level directory indexes don't exist until the first file is created => they do not need initialization)
    DIRE1.dire.init();
    // if the write of the first level directory index block to the partition failed, return an error code
    if( cache.writeToPart(part, RootIndx1Location, DIRE1) != MFS_OK ) return MFS_ERROR;

    // reserve the first two blocks on the partition
    // this code isn't a part of the bit vector initialization because then the partition could be left in an inconsistent state if the write of the fist level directory index block failed!
    BITV.bitv.reserve(BitvLocation);
    BITV.bitv.reserve(RootIndx1Location);
    // if the write of the bit vector block to the partition failed, return an error code
    if( cache.writeToPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;

    // the partition is now formatted
    formatted = true;
    // return that the operation was successful
    return MFS_OK;
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



// allocate the number of requested blocks on the partition, append their ids if the allocation was successful
MFS KFS::allocateBlocks_uc(siz32 count, std::vector<idx32>& ids)
{
    // if the partition isn't mounted, return an error code
    if( !part ) return MFS_ERROR;
    // if the requested number of blocks is zero, return that the allocation was successful
    if( count == 0 ) return MFS_OK;

    // create a temporary bit vector block
    Block BITV;
    // the number of allocated blocks so far
    siz32 alloccnt;

    // if the read of the bit vector block from the partition failed, return an error code
    if( cache.readFromPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;
    
    // get the number of blocks on the partition
    siz32 partsize = part->getNumOfClusters();

    // for every block in the general purpose block pool
    for( idx32 idx = BlockPoolLocation;  idx < partsize && alloccnt < count;  idx++ )
    {
        // if the current block is free (isn't already taken)
        if( BITV.bitv.isFree(idx) )
        {
            // reserve the current block
            BITV.bitv.reserve(idx);

            // save the index of the current block in the block id vector
            ids.push_back(idx);

            // increase the number of allocated blocks
            alloccnt--;
        }
    }

    // if the requested number of blocks couldn't be allocated, or the bit vector block update was unsuccessful
    if( alloccnt != count || cache.writeToPart(part, BitvLocation, BITV) != MFS_OK )
    {
        // remove the newly added block ids from the block ids array
        for( idx32 i = 0; i < alloccnt; i++ )
            ids.pop_back();

        // return an error code
        return MFS_ERROR;
    }

    // return that the operation was successful
    return MFS_OK;
}

// deallocate the blocks with the given ids from the partition, return if the deallocation was successful
MFS KFS::freeBlocks_uc(const std::vector<idx32>& ids)
{
    // if the partition isn't mounted, return an error code
    if( !part ) return MFS_ERROR;
    // if there are no blocks to be deallocated, return that the deallocation was successful
    if( ids.empty() ) return MFS_OK;

    // create a temporary bit vector block
    Block BITV;

    // if the read of the bit vector block from the partition failed, return an error code
    if( cache.readFromPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;

    // get the number of blocks on the partition
    siz32 partsize = part->getNumOfClusters();

    // for every block that should be deallocated
    for( auto& idx : ids )
    {
        // if the block to be deallocated is not in the general purpose block pool, return an error code
        if( idx < BlockPoolLocation || idx >= partsize ) return MFS_ERROR;
        // release the current block (mark it as free)
        BITV.bitv.release(idx);
    }

    // if the bit vector block update was unsuccessful, return an error code
    if( cache.writeToPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;

    // return that the operation was successful
    return MFS_OK;
}



// check if the root full file path is valid (e.g. /myfile.cpp)
MFS KFS::isFullPathValid_uc(const char* filepath)
{
    // if the filepath is missing, or it isn't absolute, return invalid status code
    if( !filepath || filepath[0] != '/' ) return MFS_NOK;

    // if the full filename is invalid, return invalid status code
    if( FileDescriptor::isFullNameValid(&filepath[1]) != MFS_OK ) return MFS_NOK;

    // return that the absolute path is valid
    return MFS_OK;
}

// check if the root full file path is a match clause (special)
// "." -- matches first empty file descriptor
// ""  -- doesn't match any file
MFS KFS::isFullPathSpecial_uc(const char* filepath)
{
    // if the filepath is missing, or it isn't a special character, return invalid status code
    if( !filepath || strcmp(filepath, ".") != 0 || strcmp(filepath, "") != 0 ) return MFS_NOK;

    // return that the absolute path is a special character
    return MFS_OK;
}



// check if the mounted partition is formatted
MFS KFS::isFormatted_uc()
{
    // if the partition isn't mounted, return an error code
    if( part == nullptr ) return MFS_ERROR;

    // return if the partition is formatted
    return ( formatted ) ? MFS_OK : MFS_NOK;
}

// get the number of files in the root directory on the mounted partition
MFS32 KFS::getRootFileCount_uc()
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;

    // if the filecnt isn't an invalid (uninitialized) number, return it
    // (it is being managed by the filesystem class whenever files are added and removed)
    if( filecnt != nullsiz32 ) return filecnt;

    // create a traversal object
    Traversal t;
    // get the number of files in the partition (by trying to find an unmatchable file, traverse the entire root directory structure)
    MFS status = fileExists_uc("", t);

    // if the traversal hasn't failed (but not catastrofically), there must be something wrong
    if( status != MFS_NOK ) return MFS_ERROR;

    // save the number of traversed files (as the file count)
    filecnt = t.filesScanned;

    // return the newly calculated file count
    return filecnt;
}

// check if a file exists in the root directory on the mounted partition, if it does return that it exists and the traversal position
MFS KFS::fileExists_uc(const char* filepath, Traversal& t)
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;
    // if the filepath is invalid and not a special character, return an error code
    if( isFullPathValid_uc  (filepath) != MFS_OK
     && isFullPathSpecial_uc(filepath) != MFS_OK ) return MFS_ERROR;

    // create temporary blocks that will hold the root directory's level1 index block, one of its level2 index blocks and one of its directory blocks during the traversal
    Block INDX1, INDX2, DIRE;

    // reinitialize the traversal
    t = Traversal { };
    // set the root index location
    t.locINDX1 = RootIndx1Location;

    // current status of the search
    uns32 status = 0;
    constexpr uns32 found = (1u << 0);   // status bit -- if the file with the given path is found
    constexpr uns32 error = (1u << 1);   // status bit -- if there is an error in the search


    // if the root directory's index1 block couldn't be read, remember that an error occured
    if( cache.readFromPart(part, t.locINDX1, INDX1) != MFS_OK )   status |= error;

    // for every index2 block that the root directory's index1 block references
    for( t.idx1 = 0;   t.idx1 < IndxBlkSize && !status;   t.idx1++ )
    {
        // if the entry doesn't point to a valid index2 block, return to the previous level of the traversal
        if( (t.locINDX2 = INDX1.indx.entry[t.idx1]) == nullblk ) break;
        // if the current index2 block couldn't be read, remember that an error occured
        if( cache.readFromPart(part, t.locINDX2, INDX2) != MFS_OK ) status |= error;

        // for every directory block the current index2 block references
        for( t.idx2 = 0;   t.idx2 < IndxBlkSize && !status;   t.idx2++ )
        {
            // if the entry doesn't point to a valid directory block, return to the previous level of the traversal
            if( (t.locDIRE = INDX2.indx.entry[t.idx2]) == nullblk ) break;
            // if the current directory block couldn't be read, remember that an error occured
            if( cache.readFromPart(part, t.locDIRE, DIRE) != MFS_OK ) status |= error;

            // for every file descriptor in the current directory block
            for( t.idx3 = 0;   t.idx3 < DireBlkSize && !status;   t.idx3++ )
            {
                // if the file descriptor is not taken, return to the previous level of traversal
                if( !DIRE.dire.filedesc[t.idx3].isTaken() ) break;
                // if the given full file name matches the full file name in the file descriptor, the search is successful
                if( DIRE.dire.filedesc[t.idx3].cmpFullName(&filepath[1]) == MFS_EQUAL ) status |= found;
                // increase the number of scanned files
                t.filesScanned++;
            }
        }
    }


    // if an error has occured in the root directory traversal, return an error code
    if( status & error ) return MFS_ERROR;

    // if the file with the given path has not been found, return
    if( !(status & found) ) return MFS_NOK;

    // return that the file with the given path has been found
    return MFS_OK;
}



// open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, 'w'rite, 'a'ppend)
// +   read and append fail if the file with the given full path doesn't exist
// +   write will try to open a file before writing to it if the file doesn't exist
KFileHandle KFS::openFile_uc(const char* filepath, char mode)
{
    // if there is no mounted partition, return a null pointer
    if( !part ) return nullptr;
    // if the selected mode isn't recognized, return a null pointer
    if( mode != 'r' || mode != 'w' || mode != 'a' ) return nullptr;
    // if the filepath is invalid, return a null pointer
    if( isFullPathValid_uc(filepath) != MFS_OK ) return nullptr;

    // TODO: napisati komentare
    std::string fpath { filepath };

    if( open_files.find(fpath) == open_files.cend() )
        open_files[fpath] = KFileHandle { new KFile() };

    KFileHandle handle { open_files[fpath] };


    Traversal pos;
    MFS status = fileExists_uc(filepath, pos);
    if( status < 0 ) return nullptr;

    if( status == MFS_NOK && (mode == 'r' || mode == 'a') ) return nullptr;












    return handle;
}

// close a file with the given full file path (e.g. /myfile.cpp)
MFS KFS::closeFile_uc(KFileHandle handle)
{
    // TODO: napraviti
}

// delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
MFS KFS::deleteFile_uc(const char* filepath)
{
    // if there is no mounted partition, return an error code
    if( !part ) return MFS_ERROR;
    // if the filepath is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return MFS_ERROR;

    // TODO: napraviti
    MFS32 idxDIRE = fileExists_uc(filepath);
    if( idxDIRE < 0 ) return MFS_ERROR;
    if( idxDIRE == nullblk ) return MFS_OK;
    
    Block DIRE;
    // TODO: napraviti da se update-uju file pokazivaci

    return MFS_OK;
}



// read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS32 KFS::readFromFile_uc(KFileHandle handle, idx32 pos, siz32 count, Buffer buffer)
{
    // TODO: napraviti
}

// write the requested number of bytes from the buffer into the file starting from the given position
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS KFS::writeToFile_uc(KFileHandle handle, idx32 pos, siz32 count, const Buffer buffer)
{
    // TODO: napraviti
}

// throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem)
MFS KFS::truncateFile_uc(KFileHandle handle, idx32 pos)
{
    // TODO: napraviti
}





