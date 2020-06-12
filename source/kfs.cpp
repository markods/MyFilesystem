﻿// _____________________________________________________________________________________________________________________________________________
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

// FIXME: napraviti thread koji periodicno zove flush kesa (koristiti std::thread i std::time_mutex, nit prekinuti unutar nje same i uraditi join sa njom u destruktoru kfs-a!)
// FIXME: proveriti da li file handle treba okruziti sa std::atomic<> i da li treba koristiti memory barijere!


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
{
    // initialize the event mutexes to locked state (so that the threads trying to access them block)
    mutex_part_unmounted    .lock();
    mutex_all_files_closed  .lock();
    mutex_no_threads_waiting.lock();
}

// wait until there are [no threads waiting on any event] (except for the exclusive mutex), if there is at least one thread waiting wake it up
// destruct the filesystem
KFS::~KFS()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // set that the opening of new files is forbidden
    prevent_open = true;

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

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt--;
    }

    // unconditionally unmount the given partition
    // FIXME: if there is an error, then the cache has not been fully flushed to the partition, and the filesystem is corrupt!
    unmount_uc();

    // set that the filesystem is up for destruction
    up_for_destruction = true;

    // if there are threads waiting for <the partition unmount event> or the <all files closed event>
    // (there are definitely no threads waiting for the <file open event>, since the partition has been unmounted)
    while( mutex_part_unmounted_cnt + mutex_all_files_closed_cnt > 0 )
    {
        // 1. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 2. if there is a thread waiting for <all files closed> wake it up
        if     ( mutex_part_unmounted_cnt   > 0 ) { mutex_part_unmounted  .unlock(); }
        else if( mutex_all_files_closed_cnt > 0 ) { mutex_all_files_closed.unlock(); }

        // wait until there are <no threads waiting on any event>
        mutex_no_threads_waiting.lock();

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
    mutex_excl.lock();

    // loop until the blocking condition is false
    // if there is already a mounted partition, make this thread wait until it is unmounted
    while( part != nullptr )
    {
        // if the filesystem is up for destruction, skip the blocking condition
        if( up_for_destruction ) break;

        // increase the number of threads waiting for the unmount event
        mutex_part_unmounted_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the partition unmount event
        mutex_part_unmounted.lock();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for unmount event
        mutex_part_unmounted_cnt--;
    }

    // if the filesystem is up for destruction
    if( up_for_destruction )
    {
        // 1. if there is a thread waiting for <all files closed> wake it up, otherwise
        // 2. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 3. wake up the thread that waited to <destroy the partitition>
        if     ( mutex_all_files_closed_cnt > 0 ) { mutex_all_files_closed  .unlock(); }
        else if( mutex_part_unmounted_cnt   > 0 ) { mutex_part_unmounted    .unlock(); }
        else                                      { mutex_no_threads_waiting.unlock(); }

        // prevent the operation from executing (even if the thread waited)
        return MFS_DESTROY;
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

// wait until [all open files on the partition are closed]
// unmount the partition from the filesystem
// wake up a single! thread that waited for [all files the partition to be closed] or to [mount a partition], in this exact order!
MFS KFS::unmount()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // prevent the opening of new files
    prevent_open = true;

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // if the filesystem is up for destruction, skip the blocking condition
        if( up_for_destruction ) break;

        // increase the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the all files closed event
        mutex_all_files_closed.lock();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt--;
    }

    // if the filesystem is up for destruction
    if( up_for_destruction )
    {
        // 1. if there is a thread waiting for <all files closed> wake it up, otherwise
        // 2. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 3. wake up the thread that waited to <destroy the partitition>
        if     ( mutex_all_files_closed_cnt > 0 ) { mutex_all_files_closed  .unlock(); }
        else if( mutex_part_unmounted_cnt   > 0 ) { mutex_part_unmounted    .unlock(); }
        else                                      { mutex_no_threads_waiting.unlock(); }

        // prevent the operation from executing (even if the thread waited)
        return MFS_DESTROY;
    }

    // unconditionally unmount the given partition
    MFS status = unmount_uc();

    // 1. if there is a thread waiting for <all files closed event> wake it up, otherwise
    // 2. if there is a thread waiting for <partition unmounted> wake it up, otherwise
    // 3. remove the file opening prevention policy and <release exclusive access>
    if     ( mutex_all_files_closed_cnt > 0 ) {                       mutex_all_files_closed.unlock(); }
    else if( mutex_part_unmounted_cnt   > 0 ) {                       mutex_part_unmounted  .unlock(); }
    else                                      { prevent_open = false; mutex_excl            .unlock(); }

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// wait until [all open files on the partition are closed]
// format the mounted partition, if there is no mounted partition return an error
// wake up a single thread that waited for [all open files on the partition to be closed]
MFS KFS::format()
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // loop until the blocking condition is false
    // if there is already a mounted partition and there are open files, make this thread wait until all the files on it are closed
    while( part != nullptr && !open_files.empty() )
    {
        // if the filesystem is up for destruction, skip the blocking condition
        if( up_for_destruction ) break;

        // increase the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the all files closed event
        mutex_all_files_closed.lock();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the all files closed event
        mutex_all_files_closed_cnt--;
    }

    // if the filesystem is up for destruction
    if( up_for_destruction )
    {
        // 1. if there is a thread waiting for <all files closed> wake it up, otherwise
        // 2. if there is a thread waiting for <partition unmount> wake it up, otherwise
        // 3. wake up the thread that waited to <destroy the partitition>
        if     ( mutex_all_files_closed_cnt > 0 ) { mutex_all_files_closed  .unlock(); }
        else if( mutex_part_unmounted_cnt   > 0 ) { mutex_part_unmounted    .unlock(); }
        else                                      { mutex_no_threads_waiting.unlock(); }

        // prevent the operation from executing (even if the thread waited)
        return MFS_DESTROY;
    }

    // unconditionally format the given partition
    MFS status = format_uc();

    // 1. if there is a thread waiting for <all files closed event> wake it up, otherwise
    // 2. <release exclusive access>
    if  ( mutex_all_files_closed_cnt > 0 ) { mutex_all_files_closed.unlock(); }
    else                                   { mutex_excl            .unlock(); }

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



// check if a file exists in the root directory on the mounted partition
MFS KFS::fileExists(const char* filepath)
{
    // if the full file path is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return MFS_BADARGS;

    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // create a traversal path
    Traversal t;
    // unconditionally check if the file exists (the traversal position is not needed)
    MFS status = findFile_uc(filepath, t);

    // release exclusive access
    mutex_excl.unlock();

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
    mutex_excl.lock();

    // unconditionally try to get a file handle from the open file table
    KFile::Handle handle = { getFileHandle_uc(filepath) };

    // loop until the blocking condition is false
    // if there is already a thread using the file, make this thread wait until the file is closed
    while( handle && handle->isReserved_uc() )
    {
        // increase the number of threads waiting for the file closed event
        handle->mutex_file_closed_cnt++;

        // release the exclusive access mutex
        mutex_excl.unlock();

        // wait for the file closed event
        handle->mutex_file_closed.lock();

        // the thread only reaches this point when some other thread wakes this thread up
        // the other thread didn't release its exclusive access, which means that this thread continues with exclusive access (transfered implicitly by the other thread)

        // decrease the number of threads waiting for the file closed event
        handle->mutex_file_closed_cnt--;
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
        if     ( handle             && handle->mutex_file_closed_cnt > 0 ) { handle->mutex_file_closed.unlock(); }
        else if( open_files.empty() && mutex_all_files_closed_cnt    > 0 ) { mutex_all_files_closed   .unlock(); }
        else                                                               { mutex_excl               .unlock(); }

        // return nullptr
        return nullptr;
    }

    // unconditionally try to open a file handle
    handle = openFileHandle_uc(filepath, mode);
    // if the handle has been opened, reserve the file handle with the given access mode
    if( handle ) handle->reserveAccess_uc(mode);

    // release exclusive access
    mutex_excl.unlock();

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
    mutex_excl.lock();

    // unconditionally try to close the file
    MFS32 status = closeFileHandle_uc(filepath);

    // if the operation was not successful, release exclusive access and return an error code
    if( status != MFS_OK ) { mutex_excl.unlock(); return MFS_ERROR; }

    // find a file handle with the given path in the open file table
    KFile::Handle handle { getFileHandle_uc(filepath) };

    // 1. if the handle exists and there is a thread waiting for <file closed> wake it up, otherwise
    // 2. if there are no open files and there is a thread waiting for <all files closed> wake it up, otherwise
    // 3. release exclusive access
    if     ( handle             && handle->mutex_file_closed_cnt > 0 ) { handle->mutex_file_closed.unlock(); }
    else if( open_files.empty() && mutex_all_files_closed_cnt    > 0 ) { mutex_all_files_closed   .unlock(); }
    else                                                               { mutex_excl               .unlock(); }

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
    mutex_excl.lock();

    // find a file handle with the given path in the open file table
    KFile::Handle handle { getFileHandle_uc(filepath) };

    // if the handle is not empty, then there must be a thread using it -- release exclusive access and return an error code
    if( handle ) { mutex_excl.unlock(); return MFS_ERROR; }

    // unconditionally try to delete the file
    MFS32 status = deleteFile_uc(filepath);

    // release exclusive access
    mutex_excl.unlock();

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
    mutex_excl.lock();

    // unconditionally try to read from the file
    MFS32 status = readFromFile_uc(file.locDIRE, file.entDIRE, file.seekpos, count, buffer, file.fd);

    // release exclusive access
    mutex_excl.unlock();

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
    mutex_excl.lock();

    // unconditionally try to write to the file
    MFS32 status = writeToFile_uc(file.locDIRE, file.entDIRE, file.seekpos, count, buffer, file.fd);

    // release exclusive access
    mutex_excl.unlock();

    // at this point this thread doesn't have exclusive access to the filesystem class
    // so it shouldn't do anything thread-unsafe (e.g. read the filesystem class state, ...)

    // return the operation status
    return status;
}

// throw away the file's contents starting from the seek position until the end of the file (but keep the file descriptor in the filesystem)
MFS KFS::truncateFile(KFile& file)
{
    // obtain exclusive access to the filesystem class instance
    mutex_excl.lock();

    // unconditionally try to truncate the file
    MFS32 status = truncateFile_uc(file.locDIRE, file.entDIRE, file.seekpos, file.fd);

    // release exclusive access
    mutex_excl.unlock();

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
            alloccnt--;
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
    if( ( t.status = alocBlocks_uc(MaxDepth - t.depth, ids) ) != MFS_OK ) return t.status;

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
            t.status = cache.writeToPart(part, t.loc[iINDX1], INDX1);

            // if the traversal is complete, skip the next bit of code
            if( finish_connecting ) break;
        }

    // end of do-while loop that always executes once
    } while( false );


    // if the traversal path is finally connected, return the success code
    if( t.status == MFS_OK ) return t.status;

    // otherwise, try to deallocate the newly allocated blocks, if the deallocation isn't successful return a serious error code (otherwise return that the allocation was unsuccessful)
    // FIXME: if this deallocation fails, the filesystem bit vector is corrupt
    //        currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //        therefore we permanently lose one or two blocks on the partition :(
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
    // FIXME: if this deallocation fails, the filesystem bit vector is corrupt
    //        currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //        therefore we permanently lose one or two blocks on the partition :(
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
    paddedDIRE .pad.filedesc.setFullName("*");   // an invalid filename

    // start the traversal from the beginning of the root directory index1 block
    t.init(RootIndx1Location, MaxDepth);

    // set the status of the search
    t.status = MFS_NOK;


    // start the search

    // if the root directory's index1 block couldn't be read, remember that an error occured
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

        // if the current index2 block couldn't be read, remember that an error occured
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

            // if the current directory block couldn't be read, remember that an error occured
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
    if( !formatted ) return MFS_ERROR;
    // if the opening of new files is forbidden, return an error code
    if( prevent_open ) return MFS_NOK;
    // if the selected mode isn't recognized, return an error code
    if( mode != 'r' && mode != 'w' && mode != 'a' ) return MFS_BADARGS;
    // if the filepath is invalid, return an error code
    if( isFullPathValid_uc(filepath) != MFS_OK ) return MFS_BADARGS;

    // check if the file with the given path exists on the partition, if there was an error during the search, return an error code
    if( (t.status = findFile_uc(filepath, t)) < 0 ) return MFS_ERROR;
    // if the file access mode was 'r'ead or 'a'ppend, return if the file exists (return if the search was successful)
    if( mode != 'w' ) return t.status;

    // the access mode from this point on is 'w'rite (because of the previous if)

    // if the search is successful (the file with the given full file path has been found)
    if( t.status == MFS_OK )
    {
        // if the file truncation was successful, return the success code, otherwise return an error code
        return ((t.status = truncateFile_uc(t.loc[iBLOCK], t.ent[iBLOCK], 0, fd)) == MFS_OK) ? MFS_OK : MFS_ERROR;
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
    return MFS_OK;
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



// read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read and the updated file descriptor
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS32 KFS::readFromFile_uc(idx32 locDIRE, idx32 entDIRE, siz32 pos, siz32 count, Buffer buffer, FileDescriptor& fd)
{
    // TODO: napraviti -- treba update-ovati file descriptor!
    return 0;
}

// write the requested number of bytes from the buffer into the file starting from the given position, return the updated file descriptor
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS KFS::writeToFile_uc(idx32 locDIRE, idx32 entDIRE, siz32 pos, siz32 count, const Buffer buffer, FileDescriptor& fd)
{
    // TODO: napraviti -- treba update-ovati file descriptor indx!
    return MFS_OK;
}

// throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem), return the updated file descriptor
MFS KFS::truncateFile_uc(idx32 locDIRE, idx32 entDIRE, siz32 pos, FileDescriptor& fd)
{
    // if the partition isn't formatted, return an error code
    if( !formatted ) return MFS_ERROR;
    // create a status variable
    MFS status;

    // create a block that will hold the directory block that holds the file descriptor
    Block DIRE;
    // if the directory block holding the file descriptor couldn't be read, return an error code
    if( ( status = cache.readFromPart(part, locDIRE, DIRE) ) != MFS_OK ) return status;
    
    // get the file descriptor from the directory block
    fd = DIRE.dire.filedesc[entDIRE];
    // if the start position for the truncation is outside the file, return an error code
    if( pos > fd.filesize ) return MFS_BADARGS;
    // if the truncation isn't necessary, return that the operation is successful
    if( pos == fd.filesize ) return MFS_OK;

    // create variables that will hold the number of block to be deallocated per level for the file
    int32 indx1_delta, indx2_delta, data_delta;
    // calculate the number of blocks to be deallocated for all file structure levels
    fd.getResizeBlockDelta(pos, indx1_delta, indx2_delta, data_delta);

    // create a vector for holding ids of blocks to be deallocated
    std::vector<idx32> ids;

    // reset the status of the entire operation
    status = MFS_NOK;


    // a do-while loop that always executes once (used because of the breaks -- if there was no loop surrounding the inner code, the code would be really messy)
    do
    {
        // if there are no data blocks to be deallocated, set that the deallocation was successful and skip the next bit of code
        if( data_delta == 0 ) { status = MFS_OK; break; }

        // the position in the file of the first data block to be deallocated
        siz32 first_datablk_for_dealloc_pos;
        // initialize it with the last byte of the file (pos-1 is the last byte of the soon to be truncated file) that belongs to a data block (the position of truncation must be greater than the tiny file size that doesn't even have a data block)
        first_datablk_for_dealloc_pos = ( pos > FileSizeT ) ? pos-1 : 0;
        // find the position in the file of the first data block to be deallocated
        FileDescriptor::getNextDataBlockStartPos(first_datablk_for_dealloc_pos);

        // create a traversal path for the file
        Traversal f;
        // initialize the file's traversal path <starting location>
        f.init(fd.indx, fd.getDepth());
        // initialize the traversal path <entries> to point to the first data block to be deallocated
        FileDescriptor::getTraversalEntries(first_datablk_for_dealloc_pos, f);
        // artificially initialize the <unused traversal entries> to zero (for the below algorithm to work for any file structure depth)
        // only the traversal locations can be invalid (nullblk), meaning that the file structure block at that level doesn't exist
        if( f.ent[iINDX1] == nullblk ) f.ent[iINDX1] = 0;
        if( f.ent[iINDX2] == nullblk ) f.ent[iINDX2] = 0;
        if( f.ent[iBLOCK] == nullblk ) f.ent[iBLOCK] = 0;
        // create a variable that will hold the (artificial) file structure depth
        int32 depth = MaxDepth;

        // create two padded blocks
        PaddedBlock paddedINDX1, paddedINDX2;
        // initialize the paddings in the padded blocks
        paddedINDX1.pad.entry = nullblk;
        paddedINDX2.pad.entry = nullblk;
        // create references to the block part of the padded blocks
        // the blocks will hold the file's level1 index block and the current level2 index block during the traversal
        Block& INDX1 { paddedINDX1.block };
        Block& INDX2 { paddedINDX2.block };
        // artificially initialize the first entries in the index blocks to be empty
        // this is an important step, since then we can pretend that the file structure always has maximum depth (so that the algorithm below doesn't have to work differently for each possible file structure depth)
        INDX1.indx.entry[0] = nullblk;
        INDX2.indx.entry[0] = nullblk;

        // update the file descriptor -- save the new file size
        fd.filesize = pos;


        // find all blocks to be deallocated

        // if the file's index1 block exists and it couldn't be read, remember that an error occured ######
        if( f.loc[iINDX1] != nullblk && cache.readFromPart(part, f.loc[iINDX1], INDX1) != MFS_OK ) status = MFS_ERROR;

        // for every entry in the file's (sometimes artificial) index1 block
        for( ;   status == MFS_NOK;   f.ent[iINDX1]++ )
        {
            // if the <structure depth is three>, the search started from the <first entry> in the index1 block and the index1 block <exists>
            if( depth == (1 + iINDX1) && f.ent[iINDX1] == 0 && f.loc[iINDX1] != nullblk )
            {
                // save that the index1 block should be deallocated
                ids.push_back(f.loc[iINDX1]);
                // set the file descriptor general purpose index to point to the first index2 block
                fd.indx = INDX1.indx.entry[0];
                // decrease the file structure depth
                depth--;
            }

            // if the index1 block exists and its entry doesn't point to a valid index2 block, return to the previous level of the traversal
            if( f.loc[iINDX1] != nullblk && (f.loc[iINDX2] = INDX1.indx.entry[f.ent[iINDX1]]) == nullblk ) break;


            // if the current index2 block exists and it couldn't be read, remember that an error occured ######
            if( f.loc[iINDX2] != nullblk && cache.readFromPart(part, f.loc[iINDX2], INDX2) != MFS_OK ) status = MFS_ERROR;

            // for every entry in the file's (sometimes artificial) current index2 block
            for( ;   status == MFS_NOK;   f.ent[iINDX2]++ )
            {
                // if the <structure depth is two>, the search started from the <first or second entry> in the index2 block and the index2 block <exists>
                if( depth == (1 + iINDX2) && f.ent[iINDX2] <= 1 && f.loc[iINDX2] != nullblk )
                {
                    // save that the index2 block should be deallocated
                    ids.push_back(f.loc[iINDX2]);
                    // set the file descriptor general purpose index to point to the first data block
                    fd.indx = INDX2.indx.entry[0];
                    // decrease the file structure depth
                    depth--;

                    // if the first data block in the file should be deallocated
                    if( f.ent[iINDX2] == 0 )
                    {
                        // set the file descriptor general purpose index to an invalid value
                        fd.indx = nullblk;
                        // decrease the file structure depth
                        depth--;
                    }
                }

                // if the index2 block exists and its entry doesn't point to a valid data block, return to the previous level of the traversal
                if( f.loc[iINDX2] != nullblk && (f.loc[iBLOCK] = INDX2.indx.entry[f.ent[iINDX2]]) == nullblk ) break;


                // the data block should always be deallocated
                ids.push_back(f.loc[iBLOCK]);

                // if there are no more data blocks for deallocation, finish the deallocation
                if( --data_delta == 0 ) status = MFS_OK;
            }

            // reset the index2 entry to zero
            f.ent[iINDX2] = 0;
        }
    }
    // end of do-while loop that always executes once
    while( false );

    // if the operation wasn't successful, return its status code
    if( status != MFS_OK ) return status;


    // update the file descriptor in the directory block
    DIRE.dire.filedesc[entDIRE] = fd;

    // if the write of the file descriptor to the cache wasn't successful, return its status code
    if( ( status = cache.writeToPart(part, locDIRE, DIRE) ) != MFS_OK ) return status;

    // deallocate all the blocks that are not needed anymore (after the truncation)
    // FIXME: if this deallocation fails, the filesystem bit vector is corrupt
    //        currently there is no way to guarantee atomicity of all filesystem operations, since journalling isn't implemented
    //        therefore we could permanently lose many blocks on the partition :(
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
    if( handle->mutex_file_closed_cnt == 0 ) open_files.erase(handle->filepath);

    // close the file handle
    handle.reset();

    // return that the handle was successfully closed
    return MFS_OK;
}





