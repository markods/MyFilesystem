// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS
// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS
// _____________________________________________________________________________________________________________________________________________
// KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS...KFS

#pragma once
#include <mutex>
#include <thread>
#include <string>
#include <unordered_map>
#include "!global.h"
#include "cache.h"
class Partition;
class KFile;



// kernel's implementation of a filesystem
// the filesystem only has a single (root) directory and a maximum of one mounted partition at any time
// all the public methods in this class are guarded, except for the constructor
// +   the destructor must not be called before all the threads finish using this object, and no new threads start using this object during its destruction!
// +   otherwise it could happen that a thread is using a deleted object (since we can't guarantee that the thread that called the destructor is the last one that gets woken up on the mutex)
class KFS
{
private:
    Partition* part = nullptr;        // pointer to the mounted partition
    bool formatted  = false;          // bool that tells if the partition is formatted or not
    siz32 filecnt   = nullsiz32;      // cached number of files on the partition (instead of looking through the entire partition for files to count, take this cached copy)
    Cache cache {InitialCacheSize};   // filesystem block cache, used for faster access to disk

    // global open file table, used for thread-safe access to the files in the filesystem (currently a file can be accessed by at most one thread at any given time)
    std::unordered_map<std::string, std::thread::id> oft;
    
    // mutex used for exclusive access to the filesystem class
    std::mutex m_excl;
    // mutex used for signalling partition unmount events
    std::mutex m_part_not_mounted;
    // mutex used for signalling file closed events
    std::mutex m_file_open;


public:
    // construct the filesystem
    KFS();
    // destruct the filesystem
    ~KFS();

private:
    // mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
    // if there is already a mounted partition, make the thread that wants to mount another partition wait until the old partition is unmounted
    MFS _mount(Partition* part);
    // unmount the partition from the filesystem
    // wake up a single thread that waited to mount another partition
    MFS _unmount();

    // format the mounted partition
    MFS _format();
    // check if the mounted partition is formatted
    MFS _isFormatted() const;

    // open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, 'w'rite, 'a'ppend)
    // +   read and append fail if the file with the given full path doesn't exist
    // +   write will try to open a file before writing to it if the file doesn't exist
    // if multiple threads try to work with a file, the first one gets access and the others have to wait until the first one closed its file handle
    KFile* _openFile(const char* filepath, char mode);
    // delete a file on the mounted partition given the fill file path (e.g. /myfile.cpp)
    // if multiple threads try to work with a file, the first one gets access and the others have to wait until the first one closed its file handle
    MFS _deleteFile(const char* filepath);

    // check if a file exists in the root directory on the mounted partition, if it does return the index of its directory block in the root directory
    MFS32 _fileExists(const char* filepath);
    // get the number of files in the root directory on the mounted partition
    MFS32 _getRootFileCount();
};

