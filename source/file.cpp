// _____________________________________________________________________________________________________________________________________________
// FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...F
// _____________________________________________________________________________________________________________________________________________
// FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...F
// _____________________________________________________________________________________________________________________________________________
// FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...FILE...F

#pragma once
#include "file.h"


// construct the file object, only the filesystem can create a file
File::File(KFile::Handle kfile)
{
    // save the given kernel file handle
    this->kfile = kfile;
}

// close the file, but don't delete it from the filesystem!
File::~File()
{
    // release the kernel file handle
    kfile->~KFile();

    // clear the file handle pointer
    kfile.reset();
}



// read up to the requested number of bytes from the file starting from the seek position into the given buffer, return the number of bytes read (also update the seek position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
BytesCnt File::read(BytesCnt count, char* buffer)
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class
    MFS32 status = kfile->read(count, buffer);

    // return the number of bytes read, or an error code
    return ( status >= 0 ) ? status : MFS_FILE_NOK;
}

// write the requested number of bytes from the buffer into the file starting from the seek position (also update the seek position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
char File::write(BytesCnt count, char* buffer)
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class
    MFS status = kfile->write(count, buffer);

    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FILE_OK : MFS_FILE_NOK;
}

// throw away the file's contents starting from the seek position until the end of the file
char File::truncate()
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class
    MFS status = kfile->truncate();

    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FILE_OK : MFS_FILE_NOK;
}

// set the file seek position to the given position
// +   valid positions are in the range [0, filesize] (filesize included! -- used for writing to the end of the file)
char File::seek(BytesCnt position)
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class
    MFS status = kfile->setSeekPos(position);

    // return if the operation was successful
    return ( status == MFS_OK ) ? MFS_FILE_OK : MFS_FILE_NOK;
}



// get the current seek position
BytesCnt File::filePos()
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class, return the seek position
    return kfile->getSeekPos();
}

// check if the seek position is past the end of the file (there are no more bytes left to be read)
char File::eof()
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class
    bool status = kfile->isEof();

    // return if the operation was successful
    return ( status ) ? MFS_FILE_EOF_OK : MFS_FILE_EOF_NOK;
}

// get the file size in bytes
BytesCnt File::getFileSize()
{
    // if the kernel file handle is empty, return an error code
    if( !kfile ) return MFS_FILE_NOK;

    // transfer function call to the kernel file class
    return kfile->getSize();
}



