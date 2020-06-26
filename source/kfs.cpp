// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS
// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS
// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS

#include "kfs.h"
#include "block.h"
#include "partition.h"
#include "fd.h"
#include "traversal.h"

// TODO: make a thread that will periodically flush the cache (use std::thread and std::time_mutex, interrupt the thread within itself and make the kfs destructor join it)
// TODO: check if the file handle should be surrounded with std::atomic<> and if memory barriers should be used


// _____________________________________________________________________________________________________________________________________________
// THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLI
// _____________________________________________________________________________________________________________________________________________
// THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLI

// get an instance to the filesystem class
KFS& KFS::instance()
{
    // C++11 §6.7 [stmt.dcl] p4
    // If control enters the declaration concurrently while the variable is being initialized, the concurrent execution shall wait for completion of the initialization.

    // create a static variable
    static KFS instance;

    // return it
    return instance;
}

// construct the filesystem
KFS::KFS()
{}

// wait until there are [no threads waiting on any event] (except for exclusive access), if there is at least one thread waiting wake it up
// destruct the filesystem
KFS::~KFS()
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // set that the opening of new files is forbidden
    prevent_open = true;

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // increase the number of threads waiting for the all files closed event
        sem_all_files_closed_cnt++;

        // release the exclusive access
        sem_excl.release();

        // wait for the all files closed event
        sem_all_files_closed.acquire();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        sem_all_files_closed_cnt--;
    }

    // unconditionally unmount the given partition
    // WARNING: if there is an error, then the cache has not been fully flushed to the partition, and the filesystem is corrupt!
    unmount_uc();

    // set that the filesystem is up for destruction
    up_for_destruction = true;

    // if there are threads waiting for <the partition unmount event> or the <all files closed event>
    // (there are definitely no threads waiting for the <file open event>, since the partition has been unmounted)
    while( sem_part_unmounted_cnt + sem_all_files_closed_cnt > 0 )
    {
        // 1. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 2. if there is a thread waiting for <all files closed> wake it up
        if     ( sem_part_unmounted_cnt   > 0 ) { sem_part_unmounted  .release(); }
        else if( sem_all_files_closed_cnt > 0 ) { sem_all_files_closed.release(); }

        // wait until there are <no threads waiting on any event>
        sem_no_threads_waiting.acquire();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)
    }

    // this thread never releases its exclusive access (so that other threads wanting to access the filesystem class shouldn't try to access it after its destruction)
}



// wait until [there is no mounted partition]
// mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
MFS KFS::mount(Partition* partition)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // loop until the blocking condition is false
    // if there is already a mounted partition, make this thread wait until it is unmounted
    while( part != nullptr )
    {
        // if the filesystem is up for destruction, skip the blocking condition
        if( up_for_destruction ) break;

        // increase the number of threads waiting for the unmount event
        sem_part_unmounted_cnt++;

        // release the exclusive access
        sem_excl.release();

        // wait for the partition unmount event
        sem_part_unmounted.acquire();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for unmount event
        sem_part_unmounted_cnt--;
    }

    // if the filesystem is up for destruction
    if( up_for_destruction )
    {
        // 1. if there is a thread waiting for <all files closed> wake it up, otherwise
        // 2. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 3. wake up the thread that waited to <destroy the partitition>
        if     ( sem_all_files_closed_cnt > 0 ) { sem_all_files_closed  .release(); }
        else if( sem_part_unmounted_cnt   > 0 ) { sem_part_unmounted    .release(); }
        else                                    { sem_no_threads_waiting.release(); }

        // prevent the operation from executing (even if the thread waited)
        return MFS_DESTROY;
    }

    // unconditionally mount the given partition
    MFS status = mount_uc(partition);
    
    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// wait until [all open files on the partition are closed]
// unmount the partition from the filesystem
// wake up a single! thread that waited for [all files the partition to be closed] or to [mount a partition], in this exact order!
MFS KFS::unmount()
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // prevent the opening of new files
    prevent_open = true;

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // if the filesystem is up for destruction, skip the blocking condition
        if( up_for_destruction ) break;

        // increase the number of threads waiting for the all files closed event
        sem_all_files_closed_cnt++;

        // release the exclusive access
        sem_excl.release();

        // wait for the all files closed event
        sem_all_files_closed.acquire();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        sem_all_files_closed_cnt--;
    }

    // if the filesystem is up for destruction
    if( up_for_destruction )
    {
        // 1. if there is a thread waiting for <all files closed> wake it up, otherwise
        // 2. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 3. wake up the thread that waited to <destroy the partitition>
        if     ( sem_all_files_closed_cnt > 0 ) { sem_all_files_closed  .release(); }
        else if( sem_part_unmounted_cnt   > 0 ) { sem_part_unmounted    .release(); }
        else                                    { sem_no_threads_waiting.release(); }

        // prevent the operation from executing (even if the thread waited)
        return MFS_DESTROY;
    }

    // unconditionally unmount the given partition
    MFS status = unmount_uc();

    // 1. if there is a thread waiting for <all files closed event> wake it up, otherwise
    // 2. if there is a thread waiting for <partition unmounted> wake it up, otherwise
    // 3. remove the file opening prevention policy and <release exclusive access>
    if     ( sem_all_files_closed_cnt > 0 ) {                       sem_all_files_closed.release(); }
    else if( sem_part_unmounted_cnt   > 0 ) {                       sem_part_unmounted  .release(); }
    else                                    { prevent_open = false; sem_excl            .release(); }

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// wait until [all open files on the partition are closed]
// format the mounted partition, if there is no mounted partition return an error
// wake up a single thread that waited for [all open files on the partition to be closed]
MFS KFS::format(bool deep)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // if the filesystem is up for destruction, skip the blocking condition
        if( up_for_destruction ) break;

        // increase the number of threads waiting for the all files closed event
        sem_all_files_closed_cnt++;

        // release the exclusive access
        sem_excl.release();

        // wait for the all files closed event
        sem_all_files_closed.acquire();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        sem_all_files_closed_cnt--;
    }

    // if the filesystem is up for destruction
    if( up_for_destruction )
    {
        // 1. if there is a thread waiting for <all files closed> wake it up, otherwise
        // 2. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 3. wake up the thread that waited to <destroy the partitition>
        if     ( sem_all_files_closed_cnt > 0 ) { sem_all_files_closed  .release(); }
        else if( sem_part_unmounted_cnt   > 0 ) { sem_part_unmounted    .release(); }
        else                                    { sem_no_threads_waiting.release(); }

        // prevent the operation from executing (even if the thread waited)
        return MFS_DESTROY;
    }

    // unconditionally format the given partition
    MFS status = format_uc(deep);

    // 1. if there is a thread waiting for <all files closed event> wake it up, otherwise
    // 2. <release exclusive access>
    if  ( sem_all_files_closed_cnt > 0 ) { sem_all_files_closed.release(); }
    else                                 { sem_excl            .release(); }

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}



// check if the mounted partition is formatted
MFS KFS::isFormatted()
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally check if the partition is formatted
    MFS status = isFormatted_uc();

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// get the number of files in the root directory on the mounted partition
MFS32 KFS::getRootFileCount()
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally check if the file exists
    MFS32 status = getRootFileCount_uc();

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}



// check if a file exists in the root directory on the mounted partition
MFS KFS::fileExists(const char* filepath)
{
    // if the full file path is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return MFS_BADARGS;

    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // create a traversal path
    Traversal t;
    // unconditionally check if the file exists (the traversal position is not needed)
    MFS status = findFile_uc(filepath, t);

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// wait until [no one uses the file with the given full file path]
// open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and access mode ('r'ead, 'w'rite + read, 'a'ppend + read)
// +   read and append fail if the file with the given full path doesn't exist
// +   write will try to create a file before writing to it if the file doesn't exist
KFile::Handle KFS::openFile(const char* filepath, char mode)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally try to get a file handle from the open file table
    KFile::Handle handle = { getFileHandle_uc(filepath) };

    // loop until the blocking condition is false
    // if there is already a thread using the file, make this thread wait until the file is closed
    while( handle && handle->isReserved_uc() )
    {
        // increase the number of threads waiting for the file closed event
        handle->sem_file_closed_cnt++;

        // release the exclusive access
        sem_excl.release();

        // wait for the file closed event
        handle->sem_file_closed.acquire();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the file closed event
        handle->sem_file_closed_cnt--;
    }

    // if opening of new files is prevented
    if( prevent_open )
    {
        // unconditionally try to close the file
        // IMPORTANT: this operation must succeed, otherwise there will be a deadlock! (since this operation always succeeds by design, that is not a problem)
        closeFileHandle_uc(filepath);

        // 1. if the handle exists and there is a thread waiting for <file closed> wake it up, otherwise
        // 2. if all files have been closed and there is a thread waiting for <all files closed> wake it up, otherwise
        // 3. <release exclusive access>
        if     ( handle             && handle->sem_file_closed_cnt > 0 ) { handle->sem_file_closed.release(); }
        else if( open_files.empty() && sem_all_files_closed_cnt    > 0 ) { sem_all_files_closed   .release(); }
        else                                                             { sem_excl               .release(); }

        // return nullptr
        return nullptr;
    }

    // unconditionally try to open a file handle
    handle = openFileHandle_uc(filepath, mode);
    // if the handle has been opened, reserve the file handle with the given access mode
    if( handle ) handle->reserveAccess_uc(mode);

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the file handle
    return handle;
}

// close a file with the given full file path (e.g. /myfile.cpp)
// wake up a single thread that waited to [open the now closed file]
MFS KFS::closeFile(const char* filepath)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally try to close the file
    MFS32 status = closeFileHandle_uc(filepath);

    // if the operation was not successful, release exclusive access and return an error code
    if( status != MFS_OK ) { sem_excl.release(); return MFS_ERROR; }

    // find a file handle with the given path in the open file table
    KFile::Handle handle { getFileHandle_uc(filepath) };

    // 1. if the handle exists and there is a thread waiting for <file closed> wake it up, otherwise
    // 2. if there are no open files and there is a thread waiting for <all files closed> wake it up, otherwise
    // 3. release exclusive access
    if     ( handle             && handle->sem_file_closed_cnt > 0 ) { handle->sem_file_closed.release(); }
    else if( open_files.empty() && sem_all_files_closed_cnt    > 0 ) { sem_all_files_closed   .release(); }
    else                                                             { sem_excl               .release(); }

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
// the delete will succeed only if the file is not being used by a thread (isn't open)
MFS KFS::deleteFile(const char* filepath)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // find a file handle with the given path in the open file table
    KFile::Handle handle { getFileHandle_uc(filepath) };

    // if the handle is not empty, then there must be a thread using it -- release exclusive access and return an error code
    if( handle ) { sem_excl.release(); return MFS_ERROR; }

    // unconditionally try to delete the file
    MFS32 status = deleteFile_uc(filepath);

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}



// read up to the requested number of bytes from the file starting from the seek position into the given buffer, return the number of bytes read (also update the seek position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS32 KFS::readFromFile(KFile& file, siz32 count, Buffer buffer)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally try to read from the file
    MFS32 status = readFromFile_uc(file.locDIRE, file.entDIRE, file.seekpos, count, buffer, file.fd);

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// write the requested number of bytes from the buffer into the file starting from the seek position (also update the seek position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS KFS::writeToFile(KFile& file, siz32 count, const Buffer buffer)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally try to write to the file
    MFS32 status = writeToFile_uc(file.locDIRE, file.entDIRE, file.seekpos, count, buffer, file.fd);

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// throw away the file's contents starting from the seek position until the end of the file (but keep the file descriptor in the filesystem)
MFS KFS::truncateFile(KFile& file)
{
    // obtain exclusive access to the filesystem class instance
    sem_excl.acquire();

    // unconditionally try to truncate the file
    MFS32 status = truncateFile_uc(file.locDIRE, file.entDIRE, file.seekpos, file.fd);

    // release exclusive access
    sem_excl.release();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
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
    // if there is already a mounted partition, return that the new partition cannot be mounted
    if( part ) return MFS_NOK;

    // create a block that will be used as a bit vector block
    Block BITV;
    // if the bit vector block can't be read from the new partition, return an error code
    if( cache.readFromPart(partition, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;
    
    // mount the new partition
    part = partition;
    // check if the new partition is formatted
    // (the first two bits of the bit vector block have to be both set if the partition is formatted,
    // since the first block on the partition is always occupied by the bit vector, and the second by the root directory's first level index block)
    formatted = BITV.bitv.isTaken(BitvLocation) && BITV.bitv.isTaken(RootIndx1Location);
    // reset the previous file count to an invalid value
    filecnt = nullsiz32;

    // return that the operation was successful
    return MFS_OK;
}

// unmount the partition from the filesystem
MFS KFS::unmount_uc()
{
    // if there is already no mounted partition present, return that the operation was successful
    if( part == nullptr ) return MFS_OK;

    // free all the slots from the cache that come from this mounted partition (since there is at most one mounted partition at any given time, free every occupied block in the cache)
    cache.freeThisManySlots(part, cache.getSlotCount() - cache.getFreeCount());
    // if all the slots couldn't be freed, return an error code (if we skipped this step, we couldn't flush the cache slots that to the given partition later on)
    if( cache.getFreeCount() != cache.getSlotCount() ) return MFS_ERROR;

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
MFS KFS::format_uc(bool deep)
{
    // if the partition isn't mounted, return an error code
    if( !part ) return MFS_ERROR;

    // create a temporary bit vector block and a temporary level 1 index block (of the root directory)
    Block BITV, INDX1;

    // initialize the bit vector on the partition (to all zeros)
    BITV.bitv.init();
    // if the write of the bit vector block to the partition failed, return an error code
    if( cache.writeToPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;

    // initialize the first level directory index on the partition
    // (second level directory indexes don't exist until the first file is created => they do not need initialization)
    INDX1.indx.init();
    // if the write of the first level directory index block to the partition failed, return an error code
    if( cache.writeToPart(part, RootIndx1Location, INDX1) != MFS_OK ) return MFS_ERROR;

    // reserve the first two blocks on the partition
    // this code isn't a part of the bit vector initialization because then the partition could be left in an inconsistent state if the write of the fist level directory index block failed!
    BITV.bitv.reserve(BitvLocation);
    BITV.bitv.reserve(RootIndx1Location);
    // if the write of the bit vector block to the partition failed, return an error code
    if( cache.writeToPart(part, BitvLocation, BITV) != MFS_OK ) return MFS_ERROR;

    // if the general purpose block pool should be formatted as well
    if( deep )
    {
        // create a data block
        Block DATA;
        // initialize it to all zeros
        DATA.data.init();

        // get the number of blocks on the partition
        siz32 blockcnt = part->getNumOfClusters();

        // for all the blocks in the general purpose block pool on the partition
        for( idx32 loc = BlockPoolLocation; loc < blockcnt; loc++ )
        {
            // if the write of the data block to the partition failed, return an error code
            if( cache.writeToPart(part, loc, DATA) != MFS_OK ) return MFS_ERROR;
        }
    }

    // the partition is now formatted
    formatted = true;
    // reset the partition file count to zero
    filecnt = 0;

    // return that the operation was successful
    return MFS_OK;
}



// check if the mounted partition is formatted
MFS KFS::isFormatted_uc()
{
    // if the partition isn't mounted, return an error code
    if( !part ) return MFS_ERROR;

    // return if the partition is formatted
    return (formatted) ? MFS_OK : MFS_NOK;
}

// get the number of files in the root directory on the mounted partition
MFS32 KFS::getRootFileCount_uc()
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;

    // if the filecnt isn't an invalid (uninitialized) number, return it
    // (it is being managed by the filesystem class whenever files are added and removed)
    if( filecnt != nullsiz32 ) return filecnt;

    // create an empty traversal object
    Traversal t;

    // get the number of files in the partition (by trying to find an unmatchable file, traverse the entire root directory structure)
    MFS status = findFile_uc("", t);

    // if the traversal hasn't failed (but not catastrophically), there must be something wrong
    if( status != MFS_NOK ) return MFS_ERROR;

    // save the number of traversed files (as the file count)
    filecnt = t.filesScanned;

    // return the newly calculated file count
    return filecnt;
}



// check if the root full file path is valid (e.g. /myfile.cpp)
MFS KFS::isFullPathValid_uc(const char* filepath)
{
    // if the filepath is missing, or it isn't absolute, return an invalid status code
    if( !filepath || filepath[0] != '/' ) return MFS_NOK;

    // if the full filename is invalid, return invalid status code
    if( FileDescriptor::isFullNameValid(&filepath[1]) != MFS_OK ) return MFS_NOK;

    // return that the absolute path is valid
    return MFS_OK;
}

// check if the full file path is a special character
MFS KFS::isFullPathSpecial_uc(const char* filepath)
{
    // if the file path is missing, return an invalid status code
    if( !filepath ) return MFS_NOK;

    // create a temporary char buffer and initialize it to all null characters ('\0')
    char str[2] = {};

    // for every special character
    for( idx32 i = 0; i < special_char_cnt; i++ )
    {
        // initialize the array to 
        str[0] = special_char[i];

        // if the file path matches the special character, return that the absolute path is not special
        if( strcmp(filepath, str) == 0 ) return MFS_OK;
    }

    // return that the absolute path is not a match clause
    return MFS_NOK;
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
MFS KFS::alocBlocks_uc(siz32 count, std::vector<idx32>& ids)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
    // if the requested number of blocks is zero, return that the allocation was successful
    if( count == 0 ) return MFS_OK;

    // create a temporary bit vector block
    Block BITV;
    // the number of allocated blocks so far
    siz32 alloccnt = 0;

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
            alloccnt++;
        }
    }

    // if the requested number of blocks couldn't be allocated, or the bit vector block update was unsuccessful
    if( alloccnt != count || cache.writeToPart(part, BitvLocation, BITV) != MFS_OK )
    {
        // remove the newly added block ids from the block ids array
        for( idx32 i = 0; i < alloccnt; i++ )
            ids.pop_back();

        // if the requested number of block couldn't be allocated, return that the operation failed, otherwise return an error code
        return (alloccnt != count) ? MFS_NOK : MFS_ERROR;
    }

    // return that the operation was successful
    return MFS_OK;
}

// deallocate the blocks with the given ids from the partition, return if the deallocation was successful
MFS KFS::freeBlocks_uc(const std::vector<idx32>& ids)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
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
        // if the block to be deallocated is an invalid block, just skip it
        if( idx == nullblk ) continue;
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



// find or allocate a free file descriptor in the root directory, return its traversal position
MFS KFS::alocFileDesc_uc(Traversal& t)
{
    // try to find an empty file descriptor in the root directory
    findFile_uc(".", t);

    // if the search was successful or there was an error, return the status code
    if( t.status != MFS_NOK ) return t.status;
    // if the level 1 root index block isn't allocated return an error code
    if( t.loc[iINDX1] == nullblk ) return t.status = MFS_ERROR;
    // if the root directory is full return that the allocation failed (the root directory is full if the traversal level 1 (root) index entry is past the last entry in the block)
    if( t.ent[iINDX1] == IndxBlkSize ) return t.status = MFS_NOK;


    // vector for holding newly allocated block ids
    std::vector<idx32> ids;

    // try to allocate blocks for empty file descriptors, if the allocation wasn't successful return its status
    if( ( t.status = alocBlocks_uc((t.loc[iINDX1] == nullblk) + (t.loc[iINDX2] == nullblk) + (t.loc[iBLOCK] == nullblk), ids) ) != MFS_OK ) return t.status;

    // create blocks for holding the level 1 root index, current level 2 root index and the current root directory block
    Block INDX1, INDX2, BLOCK;
    // set the allocation status
    t.status = MFS_OK;
    // set the status of the traversal connecting process
    bool finish_connecting = false;

    // start connecting the allocated blocks


    // a do-while loop that always executes once (used because of the breaks -- if there was no loop surrounding the inner code, the code would be really messy)
    do
    {
        // FIXME: the next section of code should be a for loop (0..MaxDepth-1) instead of the unrolled loop we currently see

        // connecting the root directory block
        {
            // the directory block must be missing from the traversal path (since we are here, connecting the missing blocks)
            // therefore, always initialize the directory block (instead of reading it from the partition)
            if( true )
            {
                // initialize the directory block
                BLOCK.dire.init();
                // save the location of the directory block in the traversal (as the last - 0 newly allocated block)
                t.loc[iBLOCK] = ids.at(ids.size()-1 - iBLOCK);
                // reset the entry index in the directory block to point to the first entry inside the block
                t.ent[iBLOCK] = 0;
            }
            else
            {
                // nothing here
            }

            // if the block write to the partition failed, skip the next bit of code
            if( (t.status = cache.writeToPart(part, t.loc[iBLOCK], BLOCK)) != MFS_OK ) break;
        }

        // connecting the level 2 root index block
        {
            // if the level 2 index block is missing in the traversal path
            if( t.loc[iINDX2] == nullblk )
            {
                // initialize the level 2 index block
                INDX2.indx.init();
                // save the location of the level 2 index block in the traversal (as the last - 1 newly allocated block)
                t.loc[iINDX2] = ids.at(ids.size()-1 - iINDX2);
                // reset the entry index in the level 2 index block to point to the first entry inside the block
                t.ent[iINDX2] = 0;
            }
            // otherwise (if the level 2 index block is present in the traversal path)
            else
            {
                // read the level 2 index block from the partition, if the read fails skip the next bit of code
                if( (t.status = cache.readFromPart(part, t.loc[iINDX2], INDX2)) != MFS_OK ) break;
                // set that the traversal connecting process should finish after connecting this block (since the block is present, the path is connected after the block update)
                finish_connecting = true;
            }

            // point the first empty entry in the block to the location of the directory block in the path
            INDX2.indx.entry[t.ent[iINDX2]] = t.loc[iBLOCK];
            // if the block write to the partition failed, skip the next bit of code
            if( ( t.status = cache.writeToPart(part, t.loc[iINDX2], INDX2) ) != MFS_OK ) break;

            // if the traversal is complete, skip the next bit of code
            if( finish_connecting ) break;
        }

        // connecting the level 1 root index block
        {
            // the level 1 root index block must always be present on the partition (specification simplification)
            // therefore, always read the directory block from the partition (instead of initializing it)
            if( false )
            {
                // nothing here
            }
            else
            {
                // read the level 1 index block from the partition, if the read fails skip the next bit of code
                if( (t.status = cache.readFromPart(part, t.loc[iINDX1], INDX1)) != MFS_OK ) break;
                // set that the traversal connecting process should finish after connecting this block (since the block is present, the path is connected after the block update)
                finish_connecting = true;
            }

            // point the first empty entry in the block to the location of the level 2 index block in the path
            INDX1.indx.entry[t.ent[iINDX1]] = t.loc[iINDX2];
            // if the block write to the partition failed, skip the next bit of code
            if( ( t.status = cache.writeToPart(part, t.loc[iINDX1], INDX1) ) != MFS_OK ) break;

            // if the traversal is complete, skip the next bit of code
            if( finish_connecting ) break;
        }

    // end of do-while loop that always executes once
    } while( false );


    // if the traversal path is finally connected, return the success code
    if( t.status == MFS_OK ) return t.status;

    // otherwise, try to deallocate the newly allocated blocks, if the deallocation isn't successful return a serious error code (otherwise return that the allocation was unsuccessful)
    // WARNING: if this deallocation fails, the filesystem bit vector is corrupt
    //          currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //          therefore we permanently lose one or two blocks on the partition :(
    return ( ( t.status = freeBlocks_uc(ids) ) == MFS_OK ) ? MFS_NOK : MFS_ERROR;
}

// free the file descriptor with the given traversal position in the root directory, and compact index and directory block entries afterwards (possibly deallocate them if they are empty)
MFS KFS::freeFileDesc_uc(Traversal& t)
{
    // if a previous operation wasn't successful, return an error code
    if( t.status != MFS_OK ) return MFS_BADARGS;

    // create three padded blocks
    PaddedBlock paddedINDX1, paddedINDX2, paddedBLOCK;
    // create references to the block part of the padded blocks
    // the blocks will hold the root directory's level1 index block, one of its level2 index blocks and one of its directory blocks during the traversal
    Block& INDX1 { paddedINDX1.block };
    Block& INDX2 { paddedINDX2.block };
    Block& BLOCK { paddedBLOCK.block };
    // initialize the paddings in the padded blocks
    paddedINDX1.pad.entry = nullblk;
    paddedINDX2.pad.entry = nullblk;
    paddedBLOCK.pad.filedesc.release();

    // vector for holding block ids that should be deallocated
    std::vector<idx32> ids;

    // bool that tells if the file descriptor has been removed
    bool filedesc_removed = false;


    // a do-while loop that always executes once (used because of the breaks -- if there was no loop surrounding the inner code, the code would be really messy)
    do
    {
        // FIXME: the next section of code should be a for loop (0..MaxDepth-1) instead of the unrolled loop we currently see

        // process the root directory block
        {
            // read the directory block from the partition, if the read fails skip the next bit of code
            if( (t.status = cache.readFromPart(part, t.loc[iBLOCK], BLOCK)) != MFS_OK ) break;

            // for every non-empty file descriptor after the one that's being removed
            for( idx32 idx = t.ent[iBLOCK] + 1;   BLOCK.dire.filedesc[idx].isTaken();   idx++ )
            {
                // overwrite the previous file descriptor with the current one
                BLOCK.dire.filedesc[idx-1] = BLOCK.dire.filedesc[idx];
            }

            // if the block write to the partition failed, skip the next bit of code
            if( (t.status = cache.writeToPart(part, t.loc[iBLOCK], BLOCK)) != MFS_OK ) break;

            // save that the file descriptor was removed
            filedesc_removed = true;

            // char buffer for holding the full file path
            char filepath[1 + FullFileNameSize];
            // add the forward slash to make the path absolute (hack, but works since there is only one directory -- the root directory /)
            filepath[0] = '/';

            // for every non-empty file descriptor after the one that was removed
            for( idx32 idx = t.ent[iBLOCK]; BLOCK.dire.filedesc[idx].isTaken(); idx++ )
            {
                // overwrite the full file name after the forward slash in the file path
                BLOCK.dire.filedesc[idx].getFullName(&filepath[1]);
                
                // get a file handle with the exact file path
                KFile::Handle handle { getFileHandle_uc(filepath) };

                // if the file handle exists, update its file descriptor entry (pointing to the entry in the directory block that is the file' file descriptor)
                if( handle ) handle->entDIRE = idx;
            }

            // if the first file descriptor is taken, then the directory block shouldn't be deallocated, skip the next bit of code
            if( BLOCK.dire.filedesc[0].isTaken() ) break;
        }

        // process the level 2 root index block
        {
            // read the level 2 index block from the partition, if the read fails skip the next bit of code
            if( (t.status = cache.readFromPart(part, t.loc[iINDX2], INDX2)) != MFS_OK ) break;

            // for every non-empty entry after the one that's being removed
            for( idx32 idx = t.ent[iINDX2] + 1; INDX2.indx.entry[idx] != nullblk; idx++ )
            {
                // overwrite the previous entry with the current one
                INDX2.indx.entry[idx-1] = INDX2.indx.entry[idx];
            }

            // if the block write to the partition failed, skip the next bit of code
            if( (t.status = cache.writeToPart(part, t.loc[iINDX2], INDX2)) != MFS_OK ) break;

            // since the directory block deallocation is necessary, save the id of the block to be deallocated in the block ids array
            ids.push_back(t.loc[iBLOCK]);
            // reset the location of the directory block in the traversal
            t.loc[iBLOCK] = nullblk;
            // reset the directory entry index in the traversal
            t.ent[iBLOCK] = DirectoryBlock::Size;

            // if the first entry is taken, then the level 2 index block shouldn't be deallocated, skip the next bit of code
            if( INDX2.indx.entry[0] != nullblk ) break;
        }

        // process the level 1 root index block
        {
            // read the level 1 index block from the partition, if the read fails skip the next bit of code
            if( (t.status = cache.readFromPart(part, t.loc[iINDX1], INDX1)) != MFS_OK ) break;

            // for every non-empty entry after the one that's being removed
            for( idx32 idx = t.ent[iINDX1] + 1; INDX1.indx.entry[idx] != nullblk; idx++ )
            {
                // overwrite the previous entry with the current one
                INDX1.indx.entry[idx-1] = INDX1.indx.entry[idx];
            }

            // if the block write to the partition failed, skip the next bit of code
            if( (t.status = cache.writeToPart(part, t.loc[iINDX1], INDX1)) != MFS_OK ) break;

            // since the level 2 index block deallocation is necessary, save the id of the block to be deallocated in the block ids array
            ids.push_back(t.loc[iINDX2]);
            // reset the location of the level 2 index block in the traversal
            t.loc[iINDX2] = nullblk;
            // reset the level 2 (index block) entry index in the traversal
            t.ent[iINDX2] = DirectoryBlock::Size;
        }

    // end of do-while loop that always executes once
    } while( false );

    // try to deallocate blocks that are empty
    // WARNING: if this deallocation fails, the filesystem bit vector is corrupt
    //          currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //          therefore we permanently lose one or two blocks on the partition :(
    t.status = freeBlocks_uc(ids);

    // if the file descriptor was removed, then the operation is successful (even though the empty block deallocation could have failed, the root directory isn't corrupt, what is corrupt is the bit vector)
    if( filedesc_removed ) t.status = MFS_OK;

    // return the status of the operation
    return t.status;
}



// find a file descriptor with the specified path, return the traversal position and if the find is successful
// "/file" -- find a file in the root directory
// "."     -- find the first location where an empty file descriptor should be in the root directory
// ""      -- count the number of files in the root directory (by matching a nonexistent file)
MFS KFS::findFile_uc(const char* filepath, Traversal& t)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;

    // create variables that tell if the full file path is valid or a special character
    MFS valid = MFS_NOK, special = MFS_NOK;

    // if the full file path is invalid and not a special character, return an error code
    if( ( valid   = isFullPathValid_uc  (filepath) ) != MFS_OK
     && ( special = isFullPathSpecial_uc(filepath) ) != MFS_OK )
       return MFS_BADARGS;

    // bool that says if the search is for the first location where an empty file descriptor should be
    bool find_empty_fd = (special == MFS_OK) && (filepath[0] == '\0');


    // create three padded blocks
    PaddedBlock paddedINDX1, paddedINDX2, paddedDIRE;
    // create references to the block part of the padded blocks
    // the blocks will hold the root directory's level1 index block, one of its level2 index blocks and one of its directory blocks during the traversal
    Block& INDX1 { paddedINDX1.block };
    Block& INDX2 { paddedINDX2.block };
    Block& DIRE  { paddedDIRE .block };
    // initialize the paddings in the padded blocks
    paddedINDX1.pad.entry = nullblk;
    paddedINDX2.pad.entry = nullblk;
    paddedDIRE .pad.filedesc.setFullName(".*");   // an invalid filename

    // start the traversal from the beginning of the root directory index1 block
    t.init(RootIndx1Location, MaxDepth);

    // set the status of the search
    t.status = MFS_NOK;


    // start the search
    // IMPORTANT: the 'status' variable ('ok status' when the file has been found) is the triple loop stopper!

    // if the root directory's index1 block couldn't be read, remember that an error occurred ######
    if( cache.readFromPart(part, t.loc[iINDX1], INDX1) != MFS_OK ) t.status = MFS_ERROR;

    // for every entry in the root directory's index1 block
    for( t.ent[iINDX1] = 0;   t.status == MFS_NOK;   t.ent[iINDX1]++ )
    {
        // if the entry doesn't point to a valid index2 block
        if( (t.loc[iINDX2] = INDX1.indx.entry[t.ent[iINDX1]]) == nullblk )
        {
            // if the search is for an empty file descriptor, set the status as not found (since the file descriptor hasn't been allocated)
            if( find_empty_fd ) t.status = MFS_NOK;
            // return to the previous level of the traversal
            break;
        }

        // if the current index2 block couldn't be read, remember that an error occurred ######
        if( cache.readFromPart(part, t.loc[iINDX2], INDX2) != MFS_OK ) t.status = MFS_ERROR;

        // for every entry in the current index2 block
        for( t.ent[iINDX2] = 0;   t.status == MFS_NOK;   t.ent[iINDX2]++ )
        {
            // if the entry doesn't point to a valid directory block
            if( (t.loc[iBLOCK] = INDX2.indx.entry[t.ent[iINDX2]]) == nullblk )
            {
                // if the search is for an empty file descriptor, set the status as not found (since the file descriptor hasn't been allocated)
                if( find_empty_fd ) t.status = MFS_NOK;
                // return to the previous level of the traversal
                break;
            }

            // if the current directory block couldn't be read, remember that an error occurred ######
            if( cache.readFromPart(part, t.loc[iBLOCK], DIRE) != MFS_OK ) t.status = MFS_ERROR;

            // for every file descriptor in the current directory block
            for( t.ent[iBLOCK] = 0;   t.status == MFS_NOK;   t.ent[iBLOCK]++ )
            {
                // if the given full file name matches the full file name in the file descriptor, the search is successful
                // this comparison must be before the conditional break, because sometimes we need to match an empty file descriptor!
                if( DIRE.dire.filedesc[t.ent[iBLOCK]].cmpFullName(&filepath[1]) == MFS_EQUAL ) t.status = MFS_OK;

                // if the file descriptor is free (not taken), return to the previous level of traversal
                if( DIRE.dire.filedesc[t.ent[iBLOCK]].isFree() ) break;
                
                // increase the number of scanned files (since the file descriptor is taken)
                t.filesScanned++;
            }
        }
    }


    // recalculate the depth of the traversal
    t.recalcDepth();

    // return the status of the search
    return t.status;
}

// find or create a file on the mounted partition given the full file path (e.g. /myfile.cpp) and access mode ('r'ead, 'w'rite + read, 'a'ppend + read), return the file position in the root directory and the file descriptor
// +   read and append will fail if the file with the given full path doesn't exist
// +   write will try to open a file before writing to it if the file doesn't exist
MFS KFS::createFile_uc(const char* filepath, char mode, Traversal& t, FileDescriptor& fd)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return t.status = MFS_ERROR;
    // if the opening of new files is forbidden, return an error code
    if( prevent_open ) return t.status = MFS_NOK;
    // if the selected mode isn't recognized, return an error code
    if( mode != 'r' && mode != 'w' && mode != 'a' ) return t.status = MFS_BADARGS;
    // if the filepath is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return t.status = MFS_BADARGS;

    // check if the file with the given path exists on the partition, if there was an error during the search, return an error code
    if( (t.status = findFile_uc(filepath, t)) < 0 ) return t.status = MFS_ERROR;
    // if the file access mode was 'r'ead or 'a'ppend, return if the file exists (return if the search was successful)
    if( mode != 'w' ) return t.status;

    // the access mode from this point on is 'w'rite (because of the previous if)

    // if the search is successful (the file with the given full file path has been found)
    if( t.status == MFS_OK )
    {
        // if the file truncation was successful, return the success code, otherwise return an error code
        return t.status = ((t.status = truncateFile_uc(t.loc[iBLOCK], t.ent[iBLOCK], 0, fd)) == MFS_OK) ? MFS_OK : MFS_ERROR;
    }

    // since the file doesn't exist in the root directory, try to allocate an empty file descriptor
    // if the empty file descriptor allocation was unsuccessful, return the operation status
    if( (t.status = alocFileDesc_uc(t)) != MFS_OK ) return t.status;

    // create an empty block that will hold the root directory block with the empty file descriptor
    Block DIRE;

    // if the read of the directory block is unsuccessful, return the operation status
    if( (t.status = cache.readFromPart(part, t.loc[iBLOCK], DIRE)) != MFS_OK ) return t.status;

    // reserve the empty file descriptor inside the block (initialize it with the full file name as well)
    DIRE.dire.filedesc[t.ent[iBLOCK]].reserve(&filepath[1]);

    // if the write of the directory block is unsuccessful, return the operation status
    if( (t.status = cache.writeToPart(part, t.loc[iBLOCK], DIRE)) != MFS_OK ) return t.status;

    // save the file descriptor into the given variable
    fd = DIRE.dire.filedesc[t.ent[iBLOCK]];

    // increase the partition file count
    filecnt++;

    // return that the file creation was successful
    return t.status = MFS_OK;
}

// delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
MFS KFS::deleteFile_uc(const char* filepath)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
    // if the filepath is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return MFS_BADARGS;

    // create a traversal path
    Traversal t;
    // try to find a file with the given path
    t.status = findFile_uc(filepath, t);

    // if the search encountered an error, return the operation status
    if( t.status < 0 ) return t.status;
    // if a file with the given path doesn't exist, return that the delete was successful
    if( t.status == MFS_NOK ) return t.status = MFS_OK;

    // create a temporary file descriptor
    FileDescriptor fd;
    // try to truncate the file, if the operation isn't successful return its status
    if( (t.status = truncateFile_uc(t.loc[iBLOCK], t.ent[iBLOCK], 0, fd)) != MFS_OK ) return t.status;

    // try to free the file descriptor in the root directory, if the operation isn't successful return its status
    if( (t.status = freeFileDesc_uc(t)) != MFS_OK ) return t.status;

    // decrease the partition file count
    filecnt--;

    // return that the operation was successful
    return t.status = MFS_OK;
}



// read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read and the updated position
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS32 KFS::readFromFile_uc(idx32 locDIRE, idx32 entDIRE, siz32& pos, siz32 count, Buffer buffer, FileDescriptor& fd)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
    // if the location of the directory block is not valid, return an error code
    if( locDIRE == nullblk ) return MFS_BADARGS;
    // if the entry holding the file descriptor is not valid, return an error code
    if( entDIRE == nullidx32 ) return MFS_BADARGS;
    // if the given buffer doesn't exist, return an error code
    if( !buffer ) return MFS_BADARGS;
    // if the number of bytes to be read is zero, return that the operation is successful
    if( count == 0 ) return MFS_OK;
    // create a status variable
    MFS status;

    // create a block that will hold the directory block that holds the file descriptor
    Block DIRE;
    // if the directory block holding the file descriptor couldn't be read, return an error code
    if( cache.readFromPart(part, locDIRE, DIRE) != MFS_OK ) return MFS_ERROR;
    
    // get the file descriptor from the directory block
    fd = DIRE.dire.filedesc[entDIRE];
    // if the start position for reading is outside the file, return an error code
    if( pos >= fd.filesize ) return MFS_BADARGS;

    // create a file seek position and initialize it to the given starting position
    siz32 seek = pos;
    // create a buffer write position and initialize it to zero
    siz32 read = 0;

    // make the number of bytes to be read be at most the number of available bytes (until the end of the file)
    if( count > fd.filesize - pos ) count = fd.filesize - pos;
    // reset the status of the entire operation
    status = MFS_NOK;


    // a do-while loop that always executes once (used because of the breaks -- if there was no loop surrounding the inner code, the code would be really messy)
    do
    {
        // for all the requested bytes that fit into the file descriptor
        for( ; seek < FileSizeT && count > 0; count-- )
        {
            // copy the byte from the file descriptor into the buffer and update the seek position
            buffer[read++] = fd.byte[seek++];
        }

        // if all the requested bytes have been read, skip the next bit of code
        if( count == 0 ) { status = MFS_OK; break; }

        // create a traversal path for the file
        Traversal f;
        // initialize the file's traversal path <starting location>
        f.init(fd.locINDEX, fd.getDepth());
        // initialize the traversal path <entries> to point to the first data block byte to be read
        FileDescriptor::getTraversalEntries(seek, f);
        // artificially initialize the <unused traversal entries> to zero (for the below algorithm to work for any file structure depth)
        // only the traversal locations can be invalid (nullblk), meaning that the file structure block at that level doesn't exist
        if( f.ent[iINDX1] == nullidx32 ) f.ent[iINDX1] = 0;
        if( f.ent[iINDX2] == nullidx32 ) f.ent[iINDX2] = 0;
        if( f.ent[iBLOCK] == nullidx32 ) f.ent[iBLOCK] = 0;

        // create blocks that will hold the file's index1 block, the current index2 block and the current data block during the traversal
        Block INDX1, INDX2, DATA;
        // initialize the current entry in the index blocks to point to the next block in the traversal
        INDX1.indx.entry[f.ent[iINDX1]] = f.loc[iINDX2];
        INDX2.indx.entry[f.ent[iINDX2]] = f.loc[iBLOCK];


        // read the remaining bytes into the buffer
        // IMPORTANT: the 'count' variable (the number of bytes still left to be read) is the triple loop stopper!

        // if the file's index1 block exists and it couldn't be read, remember that an error occurred and skip the next bit of code ######
        if( f.loc[iINDX1] != nullblk && cache.readFromPart(part, f.loc[iINDX1], INDX1) != MFS_OK ) { status = MFS_ERROR; break; }
        // get the index2 block location from the current index1 block entry
        f.loc[iINDX2] = INDX1.indx.entry[f.ent[iINDX1]];

        // for every entry in the file's (sometimes artificial) index1 block
        for( ;   status == MFS_NOK;   f.ent[iINDX1]++ )
        {
            // if the current index2 block exists and it couldn't be read, remember that an error occurred and skip the next bit of code ######
            if( f.loc[iINDX2] != nullblk && cache.readFromPart(part, f.loc[iINDX2], INDX2) != MFS_OK ) { status = MFS_ERROR; break; }
            // get the data block location from the current index2 block entry
            f.loc[iBLOCK] = INDX2.indx.entry[f.ent[iINDX2]];

            // for every entry in the file's (sometimes artificial) current index2 block
            for( ;   status == MFS_NOK;   f.ent[iINDX2]++ )
            {
                // if the current data block exists and it couldn't be read, remember that an error occurred and skip the next bit of code ######
                // FIXME: the data should be read directly into the buffer, but this is currently not possible since the cache's read function requires a data block as the destination (not a buffer -- char*), and a conversion from buffer to data block is not possible
                if( cache.readFromPart(part, f.loc[iBLOCK], DATA) != MFS_OK ) { status = MFS_ERROR; break; }

                // for every requested byte in the file's current data block
                for( ;   f.ent[iBLOCK] < DataBlock::Size && status == MFS_NOK;   f.ent[iBLOCK]++ )
                {
                    // copy the byte from the data block into the buffer
                    buffer[read++] = DATA.data.byte[f.ent[iBLOCK]];
                    // update the seek position
                    seek++;
                    // if all the requested bytes have been read, finish the read operation
                    if( --count == 0 ) status = MFS_OK;
                }

                // reset the data entry to zero
                f.ent[iBLOCK] = 0;
            }

            // reset the index2 entry to zero
            f.ent[iINDX2] = 0;
        }

        // reset the index1 entry to zero
        f.ent[iINDX1] = 0;
    }
    // end of do-while loop that always executes once
    while( false );

    // if the operation wasn't successful, return an error code
    if( status != MFS_OK ) return MFS_ERROR;


    // save the new seek position in the given starting position variable
    pos = seek;

    // return the number of bytes read
    return read;
}

// write the requested number of bytes from the buffer into the file starting from the given position, return the updated position and file descriptor
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS KFS::writeToFile_uc(idx32 locDIRE, idx32 entDIRE, siz32& pos, siz32 count, const Buffer buffer, FileDescriptor& fd)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
    // if the location of the directory block is not valid, return an error code
    if( locDIRE == nullblk ) return MFS_BADARGS;
    // if the entry holding the file descriptor is not valid, return an error code
    if( entDIRE == nullidx32 ) return MFS_BADARGS;
    // if the given buffer doesn't exist, return an error code
    if( !buffer ) return MFS_BADARGS;
    // if the number of bytes to be written is zero, return that the operation is successful
    if( count == 0 ) return MFS_OK;
    // if the number of bytes to be written is greater than the maximum currently available, return an error code
    if( count > FileSizeL - pos ) return MFS_ERROR;
    // create a status variable
    MFS status;

    // create a block that will hold the directory block that holds the file descriptor
    Block DIRE;
    // if the directory block holding the file descriptor couldn't be read, return an error code
    if( cache.readFromPart(part, locDIRE, DIRE) != MFS_OK ) return MFS_ERROR;

    // get the file descriptor from the directory block
    fd = DIRE.dire.filedesc[entDIRE];
    // if the start position for the write is outside the file and also not the first empty byte, return an error code
    if( pos > fd.filesize ) return MFS_BADARGS;

    // create variables that will hold the current block positions (in their corresponding levels in the file) for all the file structure levels
    siz32 indx1_pos, indx2_pos, data_pos;
    // calculate the current block positions
    FileDescriptor::getBlockCount(pos, indx1_pos, indx2_pos, data_pos);

    // create variables that will hold the thresholds (position of the first empty block in the given level) for all the file structure levels
    siz32 indx1_th, indx2_th, data_th;
    // calculate the thresholds
    FileDescriptor::getBlockCount(fd.filesize, indx1_th, indx2_th, data_th);

    // create variables that will hold the number of block to be allocated per level for the file
    int32 indx1_delta, indx2_delta, data_delta;
    // calculate the number of blocks to be allocated for all file structure levels (the numbers will be zero or positive if the file size is greater than the previous one)
    fd.getResizeBlockDelta(pos + count, indx1_delta, indx2_delta, data_delta);
    // if any of the deltas is negative, make it zero
    if( indx1_delta < 0 ) indx1_delta = 0;
    if( indx2_delta < 0 ) indx2_delta = 0;
    if(  data_delta < 0 )  data_delta = 0;

    // create a vector for holding ids of blocks to be allocated
    std::vector<idx32> ids;
    // if the allocation was not successful, return an error code
    if( alocBlocks_uc(indx1_delta + indx2_delta + data_delta, ids) != MFS_OK ) return MFS_ERROR;

    // create an index that tells the position of the first not-yet-used block in the allocated blocks vector
    siz32 iids = 0;
    // create a file seek position and initialize it to the given starting position
    siz32 seek = pos;
    // create a buffer read position and initialize it to zero
    siz32 read = 0;

    // reset the status of the entire operation
    status = MFS_NOK;


    // a do-while loop that always executes once (used because of the breaks -- if there was no loop surrounding the inner code, the code would be really messy)
    do
    {
        // for all the bytes in the buffer that fit into the file descriptor
        for( ; seek < FileSizeT && count > 0; count-- )
        {
            // copy the byte from the buffer into the file descriptor and update the seek position
            fd.byte[seek++] = buffer[read++];
        }

        // if all the given bytes have been written, skip the next bit of code
        if( count == 0 ) { status = MFS_OK; break; }

        // create a traversal path for the file
        Traversal f;
        // initialize the file's traversal path <starting location>
        f.init(fd.locINDEX, fd.getDepth());
        // initialize the traversal path <entries> to point to the last data block byte in the file (the next one is the first byte to be written to)
        FileDescriptor::getTraversalEntries(seek-1, f);
        // artificially initialize the <unused traversal entries> to zero (for the below algorithm to work for any file structure depth)
        // only the traversal locations can be invalid (nullblk), meaning that the file structure block at that level doesn't exist
        if( f.ent[iINDX1] == nullidx32 ) f.ent[iINDX1] = 0;
        if( f.ent[iINDX2] == nullidx32 ) f.ent[iINDX2] = 0;
        if( f.ent[iBLOCK] == nullidx32 ) f.ent[iBLOCK] = 0;
        // the traversal path entries can be one of the following:
        // +   nullblk
        // +   initialized (meaning valid, pointing to a valid location inside the file block structure)
        // +   uninitialized -- past the end of the file
        // we have to make sure not to use the values from the uninitialized entries, since then we will write to a block which doesn't belong to the file!

        // create blocks that will hold the file's index1 block, the current index2 block and the current data block during the traversal
        Block INDX1, INDX2, DATA;


        // write the remaining bytes into the file
        // IMPORTANT: the 'count' variable (the number of bytes still left to be written) is the triple loop stopper!

        // if the file's index1 block exists (if it is inside the file and its level exists)
        if( indx1_pos <= indx1_th && indx1_th != 0 )
        {
            // if the index1 block couldn't be read, remember that an error occurred and skip the next bit of code ######
            if( cache.readFromPart(part, f.loc[iINDX1], INDX1) != MFS_OK ) { status = MFS_ERROR; break; }
        }
        // otherwise, if the file's index1 block doesn't exist but it should exist
        else if( --indx1_delta >= 0 )
        {
            // get a block from the allocation list
            f.loc[iINDX1] = ids.at(iids++);
        }
        // otherwise, if the file's index1 block doesn't exist and should not exist
        else
        {
            // initialize the current entry in the level1 index block to point to the first level2 index block in the traversal
            INDX1.indx.entry[f.ent[iINDX1]] = f.loc[iINDX2];
        }


        // the index1 block location is now known

        // for every entry in the file's (sometimes artificial) index1 block
        for( ;   f.ent[iINDX1] < IndexBlock::Size && status == MFS_NOK;   f.ent[iINDX1]++ )
        {
            // if the file's current index2 block exists (if it is inside the file and its level exists)
            if( indx2_pos <= indx2_th && indx2_th != 0 )
            {
                // get the index2 block location from the current index1 block entry
                f.loc[iINDX2] = INDX1.indx.entry[f.ent[iINDX1]];

                // if the index2 block couldn't be read, remember that an error occurred and skip the next bit of code ######
                if( cache.readFromPart(part, f.loc[iINDX2], INDX2) != MFS_OK ) { status = MFS_ERROR; break; }
            }
            // otherwise, if the file's index2 block doesn't exist but it should exist
            else if( --indx2_delta >= 0 )
            {
                // get a block from the allocation list
                f.loc[iINDX2] = ids.at(iids++);

                // initialize the index1's current entry to point to this index2 block
                INDX1.indx.entry[f.ent[iINDX1]] = f.loc[iINDX2];
            }
            // otherwise, if the file's index2 block doesn't exist and should not exist
            else
            {
                // initialize the current entry in the level2 block to point to the first data block in the traversal
                INDX2.indx.entry[f.ent[iINDX2]] = f.loc[iBLOCK];
            }


            // the index2 block location is now known

            // for every entry in the file's (sometimes artificial) current index2 block
            for( ;   f.ent[iINDX2] < IndexBlock::Size && status == MFS_NOK;   f.ent[iINDX2]++ )
            {
                // if the file's current data block exists (if it is inside the file and its level exists)
                if( data_pos <= data_th && data_th != 0 )
                {
                    // get the data block location from the current index2 block entry
                    f.loc[iBLOCK] = INDX1.indx.entry[f.ent[iINDX1]];

                    // if the data block couldn't be read, remember that an error occurred and skip the next bit of code ######
                    if( cache.readFromPart(part, f.loc[iBLOCK], DATA) != MFS_OK ) { status = MFS_ERROR; break; }
                }
                // otherwise, if the file's data block should exist but it doesn't
                else if( --data_delta >= 0 )
                {
                    // get a block from the allocation list
                    f.loc[iBLOCK] = ids.at(iids++);

                    // initialize the index2's current entry to point to this data block
                    INDX2.indx.entry[f.ent[iINDX2]] = f.loc[iBLOCK];
                }
                // otherwise, if the file's data block should exist but it doesn't
                else
                {
                    // nothing here, since the current data block should always exist (at this point in the do-while code)
                }


                // the data block location is now known

                // for every entry that should be written to in the file's current data block
                for( ;   f.ent[iBLOCK] < DataBlock::Size && status == MFS_NOK;   f.ent[iBLOCK]++ )
                {
                    // copy the byte from the buffer into the data block
                    DATA.data.byte[f.ent[iBLOCK]] = buffer[read++];
                    // update the seek position
                    seek++;
                    // if all the requested bytes have been read, finish the read operation
                    if( --count == 0 ) status = MFS_OK;
                }


                // if the file's current data block couldn't be written to the cache, remember that an error occurred and skip the next bit of code ######
                if( cache.writeToPart(part, f.loc[iBLOCK], DATA) != MFS_OK ) { status = MFS_ERROR; break; }
                // increment the data block position
                data_pos++;

                // reset the data entry to zero (so that the next data entry iteration starts from the beginning of the data block)
                f.ent[iBLOCK] = 0;
            }


            // if the file's current index2 block exists and it couldn't be written to the cache, remember that an error occurred and skip the next bit of code ######
            if( f.loc[iINDX2] != nullblk && cache.writeToPart(part, f.loc[iINDX2], INDX2) != MFS_OK ) { status = MFS_ERROR; break; }
            // increment the level2 block position
            indx2_pos++;

            // reset the index2 entry to zero (so that the next index2 entry iteration starts from the beginning of the index2 block)
            f.ent[iINDX2] = 0;
        }


        // if the file's current index1 block exists and it couldn't be written to the cache, remember that an error occurred and skip the next bit of code ######
        if( f.loc[iINDX1] != nullblk && cache.writeToPart(part, f.loc[iINDX1], INDX1) != MFS_OK ) { status = MFS_ERROR; break; }


        // create a variable that will tell the depth of the new file structure
        siz32 depth;
        // get the depth of the written-to file
        FileDescriptor::getDepth(seek, depth);

        // update the file descriptor's general purpose index to point to the index1/index2/data block that is at the top of the file structure
        // since at least one data block must exist in the file at this point, then the depth must be greater than zero (in the range [1, 3])
        fd.locINDEX = f.loc[depth-1];
    }
    // end of do-while loop that always executes once
    while( false );


    // if the file size increased, save its new value in the file descriptor
    // it is important to do this step here, after the traversal path has been initialized (since the file structure depth depends on the file size)
    if( fd.filesize < seek) fd.filesize = seek;
    // update the file descriptor in the directory block
    DIRE.dire.filedesc[entDIRE] = fd;

    // if the write to the file was successful and the write of the file descriptor to the cache is successful
    if( status == MFS_OK && cache.writeToPart(part, locDIRE, DIRE) == MFS_OK )
    {
        // save the new seek position in the given starting position variable
        pos = seek;

        // return that the operation was successful
        return MFS_OK;
    }

    // since the operation has failed, try to deallocate all the blocks that were previously allocated
    // WARNING: if the write position was inside the file, and the write failed, the file contents are corrupt (not the file structure itself)
    //          if this deallocation fails, the filesystem bit vector is corrupt
    //          currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //          therefore we could permanently lose many blocks on the partition :(
    freeBlocks_uc(ids);

    // return an error code
    return MFS_ERROR;
}

// throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem), return the updated file descriptor
MFS KFS::truncateFile_uc(idx32 locDIRE, idx32 entDIRE, siz32 pos, FileDescriptor& fd)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
    // if the location of the directory block is not valid, return an error code
    if( locDIRE == nullblk ) return MFS_BADARGS;
    // if the entry holding the file descriptor is not valid, return an error code
    if( entDIRE == nullidx32 ) return MFS_BADARGS;
    // create a status variable
    MFS status;

    // create a block that will hold the directory block that holds the file descriptor
    Block DIRE;
    // if the directory block holding the file descriptor couldn't be read, return an error code
    if( cache.readFromPart(part, locDIRE, DIRE) != MFS_OK ) return MFS_ERROR;
    
    // get the file descriptor from the directory block
    fd = DIRE.dire.filedesc[entDIRE];
    // if the start position for the truncation is outside the file, return an error code
    if( pos > fd.filesize ) return MFS_BADARGS;
    // if the truncation isn't necessary, return that the operation is successful
    if( pos == fd.filesize ) return MFS_OK;

    // create variables that will hold the number of block to be deallocated per level for the file
    int32 indx1_delta, indx2_delta, data_delta;
    // calculate the number of blocks to be deallocated for all file structure levels (the numbers will be zero or negative since the new file size is smaller than the previous one)
    fd.getResizeBlockDelta(pos, indx1_delta, indx2_delta, data_delta);
    // flip the signs for the deltas
    indx1_delta = -indx1_delta;
    indx2_delta = -indx2_delta;
     data_delta =  -data_delta;

    // create a vector for holding ids of blocks to be deallocated
    std::vector<idx32> ids;

    // reset the status of the entire operation
    status = MFS_NOK;


    // a do-while loop that always executes once (used because of the breaks -- if there was no loop surrounding the inner code, the code would be really messy)
    do
    {
        // if there are no data blocks to be deallocated, set that the deallocation was successful and skip the next bit of code
        if( data_delta == 0 ) { status = MFS_OK; break; }

        // create a traversal path for the file
        Traversal f;
        // initialize the file's traversal path <starting location>
        f.init(fd.locINDEX, fd.getDepth());
        // initialize the traversal path <entries> to point to the last byte of the file -- the first data block to be deallocated (they are deallocated in reverse)
        FileDescriptor::getTraversalEntries(fd.filesize-1, f);
        // artificially initialize the <unused traversal entries> to zero (for the below algorithm to work for any file structure depth)
        // only the traversal locations can be invalid (nullblk), meaning that the file structure block at that level doesn't exist
        if( f.ent[iINDX1] == nullidx32 ) f.ent[iINDX1] = 0;
        if( f.ent[iINDX2] == nullidx32 ) f.ent[iINDX2] = 0;
        if( f.ent[iBLOCK] == nullidx32 ) f.ent[iBLOCK] = 0;

        // create blocks that will hold the file's level1 index block and the current level2 index block during the traversal
        Block INDX1, INDX2;
        // initialize the current entry in the index blocks to point to the next block in the traversal
        INDX1.indx.entry[f.ent[iINDX1]] = f.loc[iINDX2];
        INDX2.indx.entry[f.ent[iINDX2]] = f.loc[iBLOCK];

        // find all blocks to be deallocated starting from the end of the file
        // IMPORTANT: the 'data delta' variable (the number of data blocks to be deallocated) is the double loop stopper!

        // if the file's index1 block exists and it couldn't be read, remember that an error occurred and skip the next bit of code ######
        if( f.loc[iINDX1] != nullblk && cache.readFromPart(part, f.loc[iINDX1], INDX1) != MFS_OK ) { status = MFS_ERROR; break; }
        // get the index2 block location from the current index1 block entry
        f.loc[iINDX2] = INDX1.indx.entry[f.ent[iINDX1]];

        // if the index1 block should be deallocated
        if( --indx1_delta >= 0 )
        {
            // save that the index1 block should be deallocated
            ids.push_back(f.loc[iINDX1]);
            // if the file descriptor's general purpose index is pointing to this index1 block, point it to the first index2 block
            if( fd.locINDEX == f.loc[iINDX1] ) fd.locINDEX = INDX1.indx.entry[0];
        }

        // for every entry in the file's (sometimes artificial) index1 block
        for( ;   f.ent[iINDX1] >= 0 && status == MFS_NOK;   f.ent[iINDX1]-- )
        {
            // if the current index2 block exists and it couldn't be read, remember that an error occurred and skip the next bit of code ######
            if( f.loc[iINDX2] != nullblk && cache.readFromPart(part, f.loc[iINDX2], INDX2) != MFS_OK ) { status = MFS_ERROR; break; }
            // get the data block location from the current index2 block entry
            f.loc[iBLOCK] = INDX2.indx.entry[f.ent[iINDX2]];

            // if the index2 block should be deallocated
            if( --indx2_delta >= 0 )
            {
                // save that the index2 block should be deallocated
                ids.push_back(f.loc[iINDX2]);
                // if the file descriptor's general purpose index is pointing to this index2 block, point it to the first data block
                if( fd.locINDEX == f.loc[iINDX2] ) fd.locINDEX = INDX2.indx.entry[0];
            }

            // for every entry in the file's (sometimes artificial) current index2 block
            for( ;   f.ent[iINDX2] >= 0 && status == MFS_NOK;   f.ent[iINDX2]-- )
            {
                // the data block should always be deallocated
                ids.push_back(f.loc[iBLOCK]);
                // if the file descriptor's general purpose index is pointing to this data block, set it to an invalid value
                if( fd.locINDEX == f.loc[iINDX2] ) fd.locINDEX = nullblk;

                // if there are no more data blocks for deallocation, finish the deallocation
                if( --data_delta == 0 ) status = MFS_OK;
            }

            // reset the index2 entry to the last entry
            f.ent[iINDX2] = IndexBlock::Size-1;
        }

        // reset the index1 entry to the last entry
        f.ent[iINDX2] = IndexBlock::Size-1;
    }
    // end of do-while loop that always executes once
    while( false );

    // if the operation wasn't successful, return its status code
    if( status != MFS_OK ) return MFS_ERROR;


    // save the new file size in the file descriptor
    // it is important to do this step here, after the traversal path has been initialized (since the file structure depth depends on the file size)
    fd.filesize = pos;
    // update the file descriptor in the directory block
    DIRE.dire.filedesc[entDIRE] = fd;

    // if the write of the file descriptor to the cache wasn't successful, return its status code
    if( cache.writeToPart(part, locDIRE, DIRE) != MFS_OK ) return MFS_ERROR;

    // deallocate all the blocks that are not needed anymore (after the truncation)
    // WARNING: if this deallocation fails, the filesystem bit vector is corrupt
    //          currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //          therefore we could permanently lose many blocks on the partition :(
    freeBlocks_uc(ids);

    // return that the operation was successful
    return MFS_OK;
}



// open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, read + 'w'rite, read + 'a'ppend)
// return the file's handle with the mode unchanged if it already exists, otherwise initialize it
KFile::Handle KFS::openFileHandle_uc(const char* filepath, char mode)
{
    // if the partition isn't formatted, return nullptr
    if( !formatted ) return nullptr;
    // if the opening of new files is forbidden, return nullptr
    if( prevent_open ) return nullptr;
    // if the selected mode isn't recognized, return nullptr
    if( mode != 'r' && mode != 'w' && mode != 'a' ) return nullptr;
    // if the filepath is invalid, return nullptr
    if( isFullPathValid_uc(filepath) != MFS_OK ) return nullptr;

    // create a string from the full file path char array
    std::string path { filepath };

    // get a file handle from the open file table, for the file with the given path
    KFile::Handle handle = getFileHandle_uc(filepath);

    // if the handle exists, return it
    if( handle ) return handle;

    // create a traversal path
    Traversal t;
    // create a file descriptor
    FileDescriptor fd;
    // initialize the file descriptor
    fd.reserve(&filepath[1]);

    // find or create a file on the partition, if the operation isn't successful return nullptr
    if( (t.status = createFile_uc(filepath, mode, t, fd)) != MFS_OK ) return nullptr;

    // make a file handle for the file with the given path
    handle = KFile::Handle { new KFile(t.loc[iBLOCK], t.ent[iBLOCK], fd) };

    // add it to the open file table
    open_files[path] = handle;

    // return the handle
    return handle;
}

// get a file handle for the file with the given full file path from the filesystem open file table
KFile::Handle KFS::getFileHandle_uc(const char* filepath)
{
    // create a string from the full file path char array
    std::string path { filepath };

    // get the iterator to the file handle with the given filepath in the open file table
    auto el = open_files.find(path);
    // if the (key, value) tuple exists (if the iterator is not at the cend() location), return its second element -- value
    return (el != open_files.cend()) ? el->second : nullptr;
}

// close a file handle with the given full file path
// this operation never fails
MFS KFS::closeFileHandle_uc(const char* filepath)
{
    // find a file handle with the given path in the filesystem open file table
    KFile::Handle handle { getFileHandle_uc(filepath) };
    // if the handle is empty, return that the operation is successful
    if( !handle ) return MFS_OK;

    // release the thread's exclusive access to the handle, so that other threads can use it
    handle->releaseAccess_uc();

    // if there are no threads waiting for access to the file, remove the file handle from the open file table
    if( handle->sem_file_closed_cnt == 0 ) open_files.erase(handle->filepath);

    // close the file handle
    handle.reset();

    // return that the handle was successfully closed
    return MFS_OK;
}





