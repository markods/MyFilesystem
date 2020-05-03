// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE

#pragma once
#include "!global.h"


// kernel's implementation of a file
// this class is not guarded, since it is not meant to be shared -- it is local to the thread that constructed it
class KFile
{
private:
    friend class KFS;


private:
    // constuct the file object
    // only the filesystem can construct the file object
    KFile();

public:
    // destruct the file object
    // close the file handle, but don't delete the file in the filesystem!
    ~KFile();

    // read the requested number of bytes from the file (starting from the previous seek position) into the given buffer
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    // updates the file seek position
    MFS32 read(siz32 count, Buffer buffer);
    // write the requested number of bytes from the buffer into the file (starting from the given position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    // updates the file seek position
    MFS write(siz32 count, Buffer buffer);
    // set the file seek position to the requested position
    MFS seek(idx32 position);
    // truncate the file -- throw away its contents, but keep the file descriptor in the filesystem
    MFS truncate();

    // get the current seek position
    MFS32 seekPos();
    // check if the seek position is at the end of the file (there are no more bytes left to be read)
    MFS   isEof();
    // get the file size (in bytes)
    MFS32 size();
};

