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
using KFileHandle = std::shared_ptr<KFile>;


// kernel's implementation of a filesystem
// the filesystem only has a single (root) directory and a maximum of one mounted partition at any time
// all the public methods in this class are thread safe, but:
// +   the destructor must not be called before all threads finish using this object, and no new threads should start using this object after the destructor is called!
// +   otherwise it could happen that a thread is using a deleted object (since we can't guarantee that the thread that called the destructor gets woken up last)
class KFS
{
private:
    // class used to represent a traversal position inside the root directory block
    struct Traversal
    {
        idx32 locINDX1 { nullblk };   // location of the root directory level 1 index block on the partition
        idx32 idx1 { nullidx32 };     // index of the entry pointing to the level 2 index block

        idx32 locINDX2 { nullblk };   // location of the root directory level 2 index block on the partition
        idx32 idx2 { nullidx32 };     // index of the entry pointing to the directory block

        idx32 locDIRE { nullblk };    // location of the directory block (containing the requested file descriptor) on the partition
        idx32 idx3 { nullidx32 };     // index of the requested file descriptor

        siz32 filesScanned { 0 };     // number of files scanned during traversal
    };

private:
    Partition* part { nullptr };        // pointer to the mounted partition
    siz32 filecnt { nullsiz32 };        // cached number of files on the partition (instead of looking through the entire partition for files to count, take this cached copy)

    bool formatted { false };           // true if the partition is formatted
    bool prevent_open { false };        // prevent new files from being opened if true
    bool up_for_destruction { false };  // true if the partition is up for destruction

    // filesystem block cache, used for faster access to disk
    Cache cache { InitialCacheSize };
    // global open file table, maps a file path to a corresponding shared file instance
    std::unordered_map< std::string, KFileHandle > open_files;

    std::mutex mutex_excl;               // mutex used for exclusive access to the filesystem class
    std::mutex mutex_part_unmounted;     // mutex used for signalling partition unmount events
    std::mutex mutex_all_files_closed;   // mutex used for signalling that all files are closed on a partition

    siz32 mutex_part_unmounted_cnt   { 0 };   // number of threads waiting for the partition unmount event
    siz32 mutex_all_files_closed_cnt { 0 };   // number of threads waiting for the all files closed event


// ====== thread-safe public interface ======
public:
    // construct the filesystem
    KFS();
    // destruct the filesystem
    ~KFS();

    // wait until there is no mounted partition
    // mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
    MFS mount(Partition* partition);
    // wait until all the open files on the partition are closed
    // unmount the partition from the filesystem
    // wake up a single thread that waited to mount another partition
    MFS unmount();
    // wait until all the open files on the partition are closed
    // format the mounted partition, if there is no mounted partition return an error
    MFS format();

    // check if the mounted partition is formatted
    MFS isFormatted();
    // get the number of files in the root directory on the mounted partition
    MFS32 getRootFileCount();
    // check if a file exists in the root directory on the mounted partition, if it does return the index of the directory block containing its file descriptor
    MFS fileExists(const char* filepath);

    // wait until no one uses the file with the given filepath
    // open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, 'w'rite, 'a'ppend)
    // +   read and append fail if the file with the given full path doesn't exist
    // +   write will try to open a file before writing to it if the file doesn't exist
    KFileHandle openFile(const char* filepath, char mode);
    // close a file with the given full file path (e.g. /myfile.cpp)
    // wake up a single thread that waited to open the now closed file
    MFS closeFile(KFileHandle handle);
    // delete a file on the mounted partition given the fill file path (e.g. /myfile.cpp)
    // the delete will succeed only if the file is not being used by a thread (isn't open)
    MFS deleteFile(const char* filepath);

    // read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS32 readFromFile(KFileHandle handle, idx32 pos, siz32 count, Buffer buffer);
    // write the requested number of bytes from the buffer into the file starting from the given position
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS writeToFile(KFileHandle handle, idx32 pos, siz32 count, Buffer buffer);
    // throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem)
    MFS truncateFile(KFileHandle handle, idx32 pos);


// ====== thread-unsafe methods ======
private:
    // mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
    MFS mount_uc(Partition* partition);
    // unmount the partition from the filesystem
    MFS unmount_uc();
    // format the mounted partition, if there is no mounted partition return an error
    MFS format_uc();

    // allocate the number of requested blocks on the partition, append their ids if the allocation was successful
    MFS allocateBlocks_uc(siz32 count, std::vector<idx32>& ids);
    // deallocate the blocks with the given ids from the partition, return if the deallocation was successful
    MFS freeBlocks_uc(const std::vector<idx32>& ids);

    // check if the root full file path is valid (e.g. /myfile.cpp)
    static MFS isFullPathValid_uc(const char* filepath);
    // check if the root full file path is a match clause (special)
    // "." -- matches first empty file descriptor
    // ""  -- doesn't match any file
    static MFS isFullPathSpecial_uc(const char* filepath);

    // check if the mounted partition is formatted
    MFS isFormatted_uc();
    // get the number of files in the root directory on the mounted partition
    MFS32 getRootFileCount_uc();
    // check if a file exists in the root directory on the mounted partition, if it does return that it exists and the traversal position
    MFS fileExists_uc(const char* filepath, Traversal& t);

    // open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, 'w'rite, 'a'ppend)
    // +   read and append fail if the file with the given full path doesn't exist
    // +   write will try to open a file before writing to it if the file doesn't exist
    KFileHandle openFile_uc(const char* filepath, char mode);
    // close a file with the given full file path (e.g. /myfile.cpp)
    MFS closeFile_uc(KFileHandle handle);
    // delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
    MFS deleteFile_uc(const char* filepath);

    // read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS32 readFromFile_uc(KFileHandle handle, idx32 pos, siz32 count, Buffer buffer);
    // write the requested number of bytes from the buffer into the file starting from the given position
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS writeToFile_uc(KFileHandle handle, idx32 pos, siz32 count, const Buffer buffer);
    // throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem)
    MFS truncateFile_uc(KFileHandle handle, idx32 pos);
};

