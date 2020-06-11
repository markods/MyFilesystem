// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE

#include "kfile.h"
#include "kfs.h"


// _____________________________________________________________________________________________________________________________________________
// THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLI
// _____________________________________________________________________________________________________________________________________________
// THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLIC.INTERFACE...THREAD.SAFE.PUBLI

// construct the file object, only the filesystem can create a file
KFile::KFile(Traversal& _fdpos, FileDescriptor& _fd)
{
    // add the forward slash to make the path absolute (hack, but works since there is only one directory -- the root directory /)
    filepath[0] = '/';
    // overwrite the full file name after the forward slash in the file path
    fd.getFullName(&filepath[1]);

    // save the file descriptor position in the directory
    fdpos = _fdpos;

    // save the file descriptor
    fd = _fd;

    // initialize the event mutexes to locked state (so that the threads trying to access them block)
    mutex_file_closed.lock();
}

// destruct the file object -- close the file handle, but don't delete the file from the filesystem!
KFile::~KFile()
{
    // get an instance to the kernel filesystem class, and call its corresponding method
    KFS::instance().closeFile(filepath);
}



// read up to the requested number of bytes from the file starting from the seek position into the given buffer, return the number of bytes read (also update the seek position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS32 KFile::read(siz32 count, Buffer buffer)
{
    // get an instance to the kernel filesystem class, and call its corresponding method
    return KFS::instance().readFromFile(*this, count, buffer);
}

// write the requested number of bytes from the buffer into the file starting from the seek position (also update the seek position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
MFS KFile::write(siz32 count, Buffer buffer)
{
    // get an instance to the kernel filesystem class, and call its corresponding method
    return KFS::instance().writeToFile(*this, count, buffer);
}

// throw away the file's contents starting from the seek position until the end of the file (but keep the file descriptor in the filesystem)
MFS KFile::truncate()
{
    // get an instance to the kernel filesystem class, and call its corresponding method
    return KFS::instance().truncateFile(*this);
}

// set the file seek position to the given position
// +   valid positions are in the range [0, filesize] (filesize included! -- used for writing to the end of the file)
MFS KFile::setSeekPos(idx32 position)
{
    // if the seek position is not in the range [0, filesize], return an error code
    if( position > fd.filesize ) return MFS_BADARGS;

    // save the new seek position
    seekpos = position;
    
    // return that the operation was successful
    return MFS_OK;
}



// get the current seek position
idx32 KFile::getSeekPos() { return seekpos; }

// check if the seek position is past the end of the file (there are no more bytes left to be read)
bool KFile::isEof() { return seekpos == fd.filesize; }

// get the file size in bytes
siz32 KFile::getSize() { return fd.filesize; }






// _____________________________________________________________________________________________________________________________________________
// THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS
// _____________________________________________________________________________________________________________________________________________
// THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS...THREAD.UNSAFE.METHODS

// reserve access to the file with the given mode (so that other threads cannot use the file before it is released)
MFS KFile::reserveAccess_uc(char mode)
{
    // if the mode isn't recognized, return an error code
    if( mode != 'r' && mode != 'w' && mode != 'a' ) return MFS_BADARGS;
    
    // set the access mode for the file
    this->mode = mode;

    // if the access mode is append, set the seek position at the end of the file, otherwise set it to the beginning of the file
    if( mode == 'a' ) seekpos = fd.filesize;
    else              seekpos = 0;

    // return that the operation was successful
    return MFS_OK;
}

// release access to the file (so that other threads can use the file)
// reset the access mode to an invalid value
void KFile::releaseAccess_uc() { mode = '\0'; }



// check if the file is reserved
// return if the access mode is specified (meaning that a thread is currently using the file)
bool KFile::isReserved_uc() { return mode != '\0'; }

// get the depth of the file structure based on the file size
// return the depth of the file structure (as calculated by the file descriptor class)
siz32 KFile::getDepth_uc() { return fd.getDepth(); }

