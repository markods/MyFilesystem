// _____________________________________________________________________________________________________________________________________________
// FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...F
// _____________________________________________________________________________________________________________________________________________
// FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...F
// _____________________________________________________________________________________________________________________________________________
// FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...F

#pragma once
#include "!global.h"
#include "kfile.h"


// user's implementation of a file (a wrapper for the kernel file)
class File
{
private:
    // the filesystem class might use some of the variables in this class directly
    friend class FS;

private:
    KFile::Handle kfile { };   // kernel file handle instance


private:
    // construct the file object, only the filesystem can create a file
    File(KFile::Handle kfile);

public:
    // close the file, but don't delete it from the filesystem!
    ~File();

    // read up to the requested number of bytes from the file starting from the seek position into the given buffer, return the number of bytes read (also update the seek position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    BytesCnt read(BytesCnt count, char* buffer);
    // write the requested number of bytes from the buffer into the file starting from the seek position (also update the seek position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    char write(BytesCnt count, char* buffer);
    // throw away the file's contents starting from the seek position until the end of the file
    char truncate();
    // set the file seek position to the given position
    // +   valid positions are in the range [0, filesize] (filesize included! -- used for writing to the end of the file)
    char seek(BytesCnt position);

    // get the current seek position
    BytesCnt filePos();
    // check if the seek position is past the end of the file (there are no more bytes left to be read)
    char eof();
    // get the file size in bytes
    BytesCnt getFileSize();
};

