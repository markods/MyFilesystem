// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR

#pragma once
#include <iosfwd>
#include "!global.h"


// structure that represents a file descriptor in the filesystem
struct FileDescriptor
{
private:
    char fname[FileNameSize-1];   // filename  (possibly without '\0')
    char fext [FileExtSize-1];    // extension (possibly without '\0')
    // 1 leftover byte (in specification)
    char eos;                     // unused byte

public:
    idx32 indx;                   // multi purpose index used as first level index/second level index/data block of file depending on the file size
    siz32 filesize;               // size of file in bytes

    // 12 leftover bytes (in specification)
    uns8 byte[FileSizeT];         // first bytes of file, that fit in the file descriptor


private:
    // reset the file name and extension fields to their defaults
    void resetFullName();

public:
    // reserve the file descriptor (initialize its fields)
    void reserve(const char* fname);
    // free the file descriptor (reset file descriptor fields to their defaults)
    void release();

    // check if the file descriptor is free
    bool isFree();
    // check if the file descriptor is taken
    bool isTaken();

    // get the depth of the file structure (depends on the file size)
    static siz32 getDepth(siz32 filesize);
    // check if the full filename is valid according to the file descriptor specification
    static MFS isFullNameValid(const char* fname);

    // copy filename and extension into given char buffer (minimal length of buffer is FullFileNameSize)
    MFS getFullName(char* buf) const;
    // compare filename and extension with given string (null terminated)
    MFS cmpFullName(const char* fname) const;

    // set filename and extension from given string (null terminated)
    MFS setFullName(const char* fname);

public:
    // print the file descriptor to the output stream
    friend std::ostream& operator<<(std::ostream& os, const FileDescriptor& fd);
};

