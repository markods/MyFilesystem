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
    char fname[FileNameSize-1];   // filename  (possibly without '\0')
    char fext [FileExtSize-1];    // extension (possibly without '\0')
    // 1 leftover byte (in specification)
    char eos;                     // null character ('\0')

public:
    idx32 indx1;                  // first level 1 index of file (not needed if filesize <= 8 + IndexBlockSize*DataBlockSize, not counting indexes!)
    siz32 filesize;               // size of file in data blocks

    // 12 leftover bytes (in specification)
    idx32 indx2;                  // first level 2 index of file (not needed if filesize <= 8B)
    uns8 byte[FileSizeS];         // first eight bytes of file (not needed if filesize = 0)


private:
    void resetFullName();   // reset the file name and extension fields to their defaults

public:
    void free();            // free the file descriptor (reset file descriptor fields to their defaults)
    bool isTaken();         // check if the file descriptor is taken (is occupied)

    // return if the full filename is valid according to the file descriptor specification
    // the filename and extension char buffers should be of at least FileNameSize and FileExtSize length
    static MFS isFullNameValid(const char* fname);

    MFS getFullName(char* buf) const;                  // copy filename and extension into given char buffer (minimal length of buffer is FullFileNameSize)
    MFS setFullName(const char* fname);                // set filename and extension from given string (null terminated)
    MFS cmpFullName(const char* fname) const;          // compare filename and extension with given string (null terminated)
    MFS cmpFullName(const FileDescriptor& fd) const;   // compare filename and extension with filename and extension in given file descriptor

    friend std::ostream& operator<<(std::ostream& os, const FileDescriptor& fd);   // print file descriptor to the output stream
};

