// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE
// _____________________________________________________________________________________________________________________________________________
// KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE...KFILE

#include "kfile.h"


// constuct the file object
// only the filesystem can construct the file object
KFile::KFile();

// destruct the file object
// close the file handle, but don't delete the file in the filesystem!
KFile::~KFile();



// read the requested number of bytes from the file (starting from the previous seek position) into the given buffer
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
// updates the file seek position
MFS32 KFile::read(siz32 count, Buffer buffer);

// write the requested number of bytes from the buffer into the file (starting from the given position)
// the caller has to provide enough memory in the buffer for this function to work correctly (at least 'count' bytes)
// updates the file seek position
MFS KFile::write(siz32 count, Buffer buffer);

// set the file seek position to the requested position
MFS KFile::seek(idx32 position);

// truncate the file -- throw away its contents, but keep the file descriptor in the filesystem
MFS KFile::truncate();



// get the current seek position
MFS32 KFile::seekPos();

// check if the seek position is at the end of the file (there are no more bytes left to be read)
MFS   KFile::isEof();

// get the file size (in bytes)
MFS32 KFile::size();



