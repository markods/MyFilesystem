// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR

#pragma once
#include <iosfwd>
#include "!global.h"
struct Traversal;


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


public:
    // check if the full filename is valid according to the file descriptor specification
    static MFS isFullNameValid(const char* fname);

    // get the depth of the file structure given the file size
    static MFS getDepth(siz32 filesize, siz32& depth);
    // get the number of blocks on all the the file structure levels given the file size
    static MFS getBlockCount(siz32 filesize, siz32& indx1_cnt, siz32& indx2_cnt, siz32& data_cnt);
    // get the number of blocks to be allocated/freed when changing from the given current file size to the given next file size
    static MFS getResizeBlockDelta(siz32 filesize_curr, siz32 filesize_next, int32& indx1_delta, int32& indx2_delta, int32& data_delta);
    
    // get the entries in the traversal path pointing to the given position in the file
    static MFS getTraversalEntries(siz32 pos, Traversal& f);
    // get the next data block start position from the given file position
    static MFS getNextDataBlockStartPos(siz32& pos);


private:
    // reset the file name and extension fields to their defaults
    void resetFullName();

public:
    // copy filename and extension into given char buffer (minimal length of buffer is FullFileNameSize)
    MFS getFullName(char* buf) const;
    // compare filename and extension with given string (null terminated)
    MFS cmpFullName(const char* fname) const;
    // set filename and extension from given string (null terminated)
    MFS setFullName(const char* fname);

    // reserve the file descriptor (initialize its fields)
    void reserve(const char* fname);
    // free the file descriptor (reset file descriptor fields to their defaults)
    void release();

    // check if the file descriptor is free
    bool isFree() const;
    // check if the file descriptor is taken
    bool isTaken() const;

    // get the depth of the file structure
    siz32 getDepth() const;
    // get the number of blocks on all the the file structure levels
    void getBlockCount(siz32& indx1_cnt, siz32& indx2_cnt, siz32& data_cnt) const;
    // get the number of blocks to be allocated/freed when changing from the current file size to the next file size
    MFS getResizeBlockDelta(siz32 filesize_next, int32& indx1_delta, int32& indx2_delta, int32& data_delta) const;

    // print the file descriptor to the output stream
    friend std::ostream& operator<<(std::ostream& os, const FileDescriptor& fd);

};

