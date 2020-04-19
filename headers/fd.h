// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR

#pragma once
#include <iosfwd>
#include "!global.h"


struct FileDescriptor
{
private:
    char fname[FileNameSize-1];   // filename  without '\0'
    char fext [FileExtSize-1];    // extension without '\0'
    // 1 leftover byte (in specification)
    char eos;                     // null character ('\0')

public:
    idx32 indx1;                  // first level 1 index of file (not needed if filesize <= 8 + IndexBlockSize*DataBlockSize, not counting indexes!)
    siz32 filesize;               // size of file in data blocks

    // 12 leftover bytes (in specification)
    idx32 indx2;                  // first level 2 index of file (not needed if filesize <= 8B)
    uns8 byte[FileSizeS];         // first eight bytes of file (not needed if filesize = 0)


private:
    void clearFullName();   // clear file name descriptor fields

public:
    void clear();           // clear file descriptor fields

    MFS getFullName(char* const buf) const;            // copy filename and extension into given char buffer (minimal length of buffer is FullFileNameSize)
    MFS setFullName(char* const str);                  // set filename and extension from given string (null terminated)
    MFS cmpFullName(char* const str) const;            // compare filename and extension with given string (null terminated)
    MFS cmpFullName(const FileDescriptor& fd) const;   // compare filename and extension with filename and extension in given file descriptor

    friend std::ostream& operator<<(std::ostream& os, const FileDescriptor& fd);   // print file descriptor to output stream
};

