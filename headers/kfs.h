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
#include "kfile.h"
class Partition;
struct FileDescriptor;
struct Traversal;


// kernel's implementation of a filesystem
// the filesystem only has a single (root) directory and a maximum of one mounted partition at any time
// all the public methods in this class are thread safe, but:
// +   the destructor must not be called before all threads finish using this object, and no new threads should start using this object after the destructor is called!
// +   otherwise it could happen that a thread is using a deleted object (since we can't guarantee that the thread that called the destructor gets woken up last)
class KFS
{
private:
    Partition* part { nullptr };        // pointer to the mounted partition
    siz32 filecnt { nullsiz32 };        // cached number of files on the partition (instead of looking through the entire partition for files to count, take this cached copy)

    bool formatted { false };           // true if the partition is formatted
    bool prevent_open { false };        // prevent new files from being opened if true
    bool up_for_destruction { false };  // true if the partition is up for destruction

    // filesystem block cache, used for faster access to disk
    Cache cache { InitialCacheSize };
    // global open file table, maps a file path to a corresponding shared file instance
    std::unordered_map< std::string, KFile::Handle > open_files;

    std::mutex mutex_excl;               // mutex used for exclusive access to the filesystem class
    std::mutex mutex_part_unmounted;     // mutex used for signalling partition unmount events
    std::mutex mutex_all_files_closed;   // mutex used for signalling that all files are closed on a partition

    siz32 mutex_part_unmounted_cnt   { 0 };   // number of threads waiting for the partition unmount event
    siz32 mutex_all_files_closed_cnt { 0 };   // number of threads waiting for the all files closed event


// ====== thread-safe public interface ======
public:
    // get an instance to the filesystem class
    static KFS& instance();

private:
    // construct the filesystem
    KFS();
    // destruct the filesystem
    ~KFS();

public:
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

    // wait until no one uses the file with the given full file path
    // open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and access mode ('r'ead, 'w'rite + read, 'a'ppend + read)
    // +   read and append fail if the file with the given full path doesn't exist
    // +   write will try to create a file before writing to it if the file doesn't exist
    KFile::Handle openFile(const char* filepath, char mode);
    // close a file with the given full file path (e.g. /myfile.cpp)
    // wake up a single thread that waited to open the now closed file
    MFS closeFile(const char* filepath);
    // delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
    // the delete will succeed only if the file is not being used by a thread (isn't open)
    MFS deleteFile(const char* filepath);

    // read up to the requested number of bytes from the file starting from the seek position into the given buffer, return the number of bytes read (also update the seek position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS32 readFromFile(KFile& file, siz32 count, Buffer buffer);
    // write the requested number of bytes from the buffer into the file starting from the seek position (also update the seek position)
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS writeToFile(KFile& file, siz32 count, const Buffer buffer);
    // throw away the file's contents starting from the seek position until the end of the file (but keep the file descriptor in the filesystem)
    MFS truncateFile(KFile& file);


// ====== thread-unsafe methods ======
private:
    // mount the partition into the filesystem (which has a maximum of one mounted partition at any time)
    MFS mount_uc(Partition* partition);
    // unmount the partition from the filesystem
    MFS unmount_uc();
    // format the mounted partition, if there is no mounted partition return an error
    MFS format_uc();

    // allocate the number of requested blocks on the partition, append their ids if the allocation was successful
    MFS alocBlocks_uc(siz32 count, std::vector<idx32>& ids);
    // deallocate the blocks with the given ids from the partition, return if the deallocation was successful
    MFS freeBlocks_uc(const std::vector<idx32>& ids);

    // find or allocate a free file descriptor in the root directory, return its traversal position
    MFS alocFileDesc_uc(Traversal& t);
    // free the file descriptor with the given traversal position in the root directory, and compact index and directory block entries afterwards (possibly deallocate them if they are empty)
    MFS freeFileDesc_uc(Traversal& t);

    // check if the root full file path is valid (e.g. /myfile.cpp)
    static MFS isFullPathValid_uc(const char* filepath);
    // check if the full file path is a special character
    static MFS isFullPathSpecial_uc(const char* filepath);

    // check if the mounted partition is formatted
    MFS isFormatted_uc();
    // get the number of files in the root directory on the mounted partition
    MFS32 getRootFileCount_uc();
    // find a file descriptor with the specified path, return the traversal position and if the find is successful
    // "/file" -- find a file in the root directory
    // "."     -- find the first location where an empty file descriptor should be in the root directory
    // ""      -- count the number of files in the root directory (by matching a nonexistent file)
    MFS findFile_uc(const char* filepath, Traversal& t);

    // open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and mode ('r'ead, read + 'w'rite, read + 'a'ppend)
    // return the file's handle with the mode unchanged if it already exists, otherwise initialize it
    KFile::Handle openFileHandle_uc(const char* filepath, char mode);
    // get a file handle for the file with the given full file path from the filesystem open file table
    KFile::Handle getFileHandle_uc(const char* filepath);
    // close a file handle with the given full file path
    MFS closeFileHandle_uc(const char* filepath);

    // find or create a file on the mounted partition given the full file path (e.g. /myfile.cpp) and access mode ('r'ead, 'w'rite + read, 'a'ppend + read), return the file position in the root directory and the file descriptor
    // +   read and append will fail if the file with the given full path doesn't exist
    // +   write will try to open a file before writing to it if the file doesn't exist
    MFS createFile_uc(const char* filepath, char mode, Traversal& t, FileDescriptor& fd);
    // delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
    MFS deleteFile_uc(const char* filepath);

    // read up to the requested number of bytes from the file starting from the given position into the given buffer, return the number of bytes read
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS32 readFromFile_uc(Traversal& t, siz32 pos, siz32 count, Buffer buffer);
    // write the requested number of bytes from the buffer into the file starting from the given position
    // the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
    MFS writeToFile_uc(Traversal& t, siz32 pos, siz32 count, const Buffer buffer);
    // throw away the file's contents starting from the given position until the end of the file (but keep the file descriptor in the filesystem)
    MFS truncateFile_uc(Traversal& t, siz32 pos);
};

