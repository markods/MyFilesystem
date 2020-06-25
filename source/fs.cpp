// _____________________________________________________________________________________________________________________________________________
// FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...F
// _____________________________________________________________________________________________________________________________________________
// FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...F
// _____________________________________________________________________________________________________________________________________________
// FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...FS...F

#include "fs.h"
#include "kfs.h"
#include "file.h"


// mount the partition into the filesystem
char FS::mount(Partition* partition)
{
    // transfer function call to the kernel filesystem class
    MFS status = KFS::instance().mount(partition);

    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FS_OK : MFS_FS_NOK;
}

// unmount the partition from the filesystem
char FS::unmount()
{
    // transfer function call to the kernel filesystem class
    MFS status = KFS::instance().unmount();

    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FS_OK : MFS_FS_NOK;
}

// format the mounted partition
char FS::format(bool deep)
{
    // transfer function call to the kernel filesystem class
    MFS status = KFS::instance().format(deep);

    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FS_OK : MFS_FS_NOK;
}



// get the number of files in the root directory on the mounted partition
FileCnt FS::readRootDir()
{
    // transfer function call to the kernel filesystem class
    MFS32 status = KFS::instance().getRootFileCount();

    // return the number of files found, or an error code
    return ( status >= 0 ) ? status : MFS_FS_ERR;
}



// check if a file exists in the root directory on the mounted partition
char FS::doesExists(const char* filepath)
{
    // transfer function call to the kernel filesystem class
    MFS status = KFS::instance().fileExists(filepath);
    
    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FS_OK : MFS_FS_NOK;
}

// open a file on the mounted partition with the given full file path (e.g. /myfile.cpp) and access mode ('r'ead, 'w'rite + read, 'a'ppend + read)
// +   read and append fail if the file with the given full path doesn't exist
// +   write will try to create a file before writing to it if the file doesn't exist
File* FS::open(const char* filepath, char mode)
{
    // transfer function call to the kernel filesystem class
    KFile::Handle handle = KFS::instance().openFile(filepath, mode);
    
    // return the file handle if the operation was successful
    return ( handle ) ? new File(handle) : nullptr;
}

// delete a file on the mounted partition given the full file path (e.g. /myfile.cpp)
char FS::deleteFile(const char* filepath)
{
    // transfer function call to the kernel filesystem class
    MFS status = KFS::instance().deleteFile(filepath);
    
    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FS_OK : MFS_FS_NOK;
}



