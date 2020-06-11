// _____________________________________________________________________________________________________________________________________________
// FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...F
// _____________________________________________________________________________________________________________________________________________
// FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...F
// _____________________________________________________________________________________________________________________________________________
// FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...F

#pragma once
#include "!global.h"
class Partition;
class File;


// user's implementation of a filesystem (a wrapper for the kernel filesystem)
// the filesystem only has a single (root) directory and a maximum of one mounted partition at any time
class FS
{
private:
    // prevent construction of the filesystem
    FS() = delete;
    // prevent destruction of the filesystem
    ~FS() = delete;


public:
    // mount the partition into the filesystem
    static char mount(Partition* partition);
    // unmount the partition from the filesystem
    static char unmount();
    // format the mounted partition
    static char format();

    // get the number of files in the root directory on the mounted partition
    static FileCnt readRootDir();

    // check if a file exists in the root directory on the mounted partition
    static char doesExists(const char* filepath);
    // open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and access mode ('r'ead, 'w'rite + read, 'a'ppend + read)
    // +   read and append fail if the file with the given full path doesn't exist
    // +   write will try to create a file before writing to it if the file doesn't exist
    static File* open(const char* filepath, char mode);
    // delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
    static char deleteFile(const char* filepath);
};

