// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE

#pragma once
#include "!global.h"
#include <mutex>
#include "traversal.h"
#include "fd.h"
class KFS;


// kernel's implementation of a file
// all the public methods in this class are thread safe
class KFile
{
private:
    // the filesystem class uses some of the variables in this class directly
    friend class KFS;

public:
    // handle that is used to access the kernel file exclusively by multiple threads
    using Handle = std::shared_ptr<KFile>;

private:
    char filepath[1 + FullFileNameSize]; // absolute path of the file
    Traversal fdpos;                     // location of the file descriptor in the directory
    FileDescriptor fd;                   // file descriptor for the file

    char mode { '\0' };                  // access mode ('r'ead, 'w'rite + read, 'a'ppend + read)
    idx32 seekpos { 0 };                 // seek position (pointer to the file contents, used in reads and writes + appends)

    std::mutex mutex_file_closed;        // mutex used for signalling that the file (handle) has been closed by a thread
    siz32 mutex_file_closed_cnt { 0 };   // number of threads waiting for the file closed event


// ====== thread-safe public interface ======
private:
    // construct the file object, only the filesystem can create a file
    KFile(Traversal& fdpos, FileDescriptor& fd);

public:
    // destruct the file object -- close the file handle, but don't delete the file from the filesystem!
    ~KFile();

    // read up to the requested number of bytes from the file starting from the seek position into the given buffer, return the number of bytes read (also update the seek position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS32 read(siz32 count, Buffer buffer);
    // write the requested number of bytes from the buffer into the file starting from the seek position (also update the seek position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS write(siz32 count, Buffer buffer);
    // throw away the file's contents starting from the seek position until the end of the file (but keep the file descriptor in the filesystem)
    MFS truncate(idx32 position);
    // set the file seek position to the given position
    // +   valid positions are in the range [0, filesize] (filesize included! -- used for writing to the end of the file)
    MFS setSeekPos(idx32 position);

    // get the current seek position
    idx32 getSeekPos();
    // check if the seek position is past the end of the file (there are no more bytes left to be read)
    bool isEof();
    // get the file size in bytes
    siz32 getSize();


// ====== thread-unsafe methods ======
private:
    // reserve access to the file with the given mode (so that other threads cannot use the file before it is released)
    MFS reserveAccess_uc(char mode);
    // release access to the file (so that other threads can use the file)
    void releaseAccess_uc();

    // check if the file is reserved
    bool isReserved_uc();
    // get the depth of the file structure based on the file size
    siz32 getDepth_uc();
};

