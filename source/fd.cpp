﻿// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR
// _____________________________________________________________________________________________________________________________________________
// FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR...FILE.DESCRIPTOR

#include <iostream>
#include <iomanip>
#include "fd.h"
using std::ostream;
using std::ios_base;
using std::setfill;
using std::setw;
using std::left;
using std::right;
using std::hex;


// reserve the file descriptor (initialize its fields)
void FileDescriptor::reserve(const char* fname)
{
    release();            // reset the file descriptor
    setFullName(fname);   // set the full filename
}

// free the file descriptor (reset file descriptor fields to their defaults)
void FileDescriptor::release()
{
    resetFullName();   // clear the descriptor fields that hold the full filename
    indx = nullblk;    // resetting multi purpose index of file to an invalid number
    filesize = 0;      // resetting filesize to zero

    // resetting first eight bytes of file
    for( uns32 i = 0; i < FileSizeT; i++ ) byte[i] = 0;
}



// check if the file descriptor is free
bool FileDescriptor::isFree() { return (fname[0] == '\0' && fext[0] == '\0'); }
// check if the file descriptor is taken
bool FileDescriptor::isTaken() { return (fname[0] != '\0' || fext[0] != '\0'); }



// get the depth of the file structure depending on the given file size
siz32 FileDescriptor::getDepth(siz32 filesize) { return (filesize > FileSizeT) + (filesize > FileSizeS) + (filesize > FileSizeM); }

// check if the full filename is valid according to the file descriptor specification
MFS FileDescriptor::isFullNameValid(const char* str)
{
    // return if the given string is nullptr
    if( !str ) return MFS_BADARGS;

    // find the length of the filename and the extension
    // examples:
    // +  filename.ext❒   ->   [filename][ext]
    //    0      7 9  12         0      7  0 2
    // +  f1.e❒           ->   [f1❒-----][e❒-]
    //    01 34                  01      7  01 
    // +  a.b.c.d.eef❒    ->   [a.b.c.d❒][eef]
    //    0     6 8  11          0     6    0 2
    uns32 n = 0;           // length of input string without '\0'
    uns32 fname_len = 0;   // length of filename     without '\0'
    uns32 fext_len  = 0;   // length of extension    without '\0'
    for( ; str[n] != '\0' && n < FullFileNameSize-1 + 1; n++ )   // n goes to FullFileNameSize on purpose (not to FullFileNameSize-1), so that we can detect if the full filename is too long
    {
        // if the filename contains any of the special characters, return that it is invalid
        for( idx32 i = 0; i < special_char_cnt; i++ )
            if( str[n] == special_char[i] )
                return MFS_NOK;

        // always increase extension length (pretend that the file is only the extension bit)
        fext_len++;

        // if the '.' (dot) is found:
        // +   then the characters that we thought belonged to the extension are part of the filename
        //     and there must be more characters that are part of the extension
        // +   therefore increase filename length by the extension length, and reset the extension length
        if( str[n] == '.' )
        {
            fname_len += fext_len;
            fext_len = 0;
        }
    }

    // if the filename is not empty, remove the last dot from it
    if( fname_len > 0 ) fname_len--;

    // the filename is invalid if:
    // +   the filename length is too long   -or-
    // +   the extension length is too long  -or-
    // +   the filename and extension lengths are both zero
    if( fname_len   > FileNameSize-1
       || fext_len  > FileExtSize-1
       || fname_len + fext_len == 0 )
        return MFS_NOK;

    // return that the filename is valid
    return MFS_OK;
}



// copy filename and extension into given char buffer (minimal length of buffer is FullFileNameSize)
MFS FileDescriptor::getFullName(char* buf) const
{
    // return if the given char buffer pointer is nullptr
    if( !buf ) return MFS_BADARGS;

    // variable which holds the full filename length
    uns32 len = 0;

    // copy filename (without extension) into buffer
    for( uns32 i = 0; fname[i] != '\0' && i < FileNameSize-1; i++ )
        buf[len++] = fname[i];

    // insert a dot character ('.') into buffer (splits filename and extension)
    buf[len++] = '.';

    // copy extension into buffer
    for( uns32 i = 0; fext[i] != '\0' && i < FileExtSize-1; i++ )
        buf[len++] = fext[i];

    // if the full filename only consists of a single dot ('.'), remove it
    if( len == 1 ) len = 0;

    // append a null character to the end of the char buffer
    buf[len++] = '\0';
    return MFS_OK;
}

// compare filename and extension with given string (null terminated)
MFS FileDescriptor::cmpFullName(const char* str) const
{
    // return if the given string is nullptr
    if( !str ) return MFS_BADARGS;

    // get the full file name from descriptor
    char full_fname[FullFileNameSize];
    getFullName(full_fname);

    // compare strings character by character
    for( uns32 i = 0; i < FullFileNameSize; i++ )
    {
        if( full_fname[i] > str[i] ) return MFS_GREATER;
        else if( full_fname[i] < str[i] ) return MFS_LESS;
    }

    // the filenames are identical
    return MFS_EQUAL;
}



// set filename and extension from given string (null terminated)
MFS FileDescriptor::setFullName(const char* str)
{
    // return if the given string is nullptr
    if( !str ) return MFS_BADARGS;

    // find the length of the filename and the extension
    // examples:
    // +  filename.ext❒   ->   [filename][ext]
    //    0      7 9  12         0      7  0 2
    // +  f1.e❒           ->   [f1❒-----][e❒-]
    //    01 34                  01      7  01 
    // +  a.b.c.d.eef❒    ->   [a.b.c.d❒][eef]
    //    0     6 8  11          0     6    0 2
    uns32 n = 0;           // length of input string without '\0'
    uns32 fname_len = 0;   // length of filename     without '\0'
    uns32 fext_len  = 0;   // length of extension    without '\0'
    for( ; str[n] != '\0' && n < FullFileNameSize-1 + 1; n++ )   // n goes to FullFileNameSize on purpose (not to FullFileNameSize-1), so that we can detect if the full filename is too long
    {
        // always increase extension length (pretend that the file is only the extension bit)
        fext_len++;

        // if the '.' (dot) is found:
        // +   then the characters that we thought belonged to the extension are part of the filename
        //     and there must be more characters that are part of the extension
        // +   therefore increase filename length by the extension length, and reset the extension length
        if( str[n] == '.' )
        {
            fname_len += fext_len;
            fext_len = 0;
        }
    }

    // if the filename is not empty, remove the last dot from it
    if( fname_len > 0 ) fname_len--;

    // the filename is invalid if:
    // +   the filename length is too long   -or-
    // +   the extension length is too long  -or-
    // +   the filename and extension lengths are both zero
    if( fname_len > FileNameSize-1
     || fext_len  > FileExtSize-1
     || fname_len + fext_len == 0 )
        return MFS_BADARGS;

    // clear the descriptor fields that hold the full filename ([filename][ext])
    resetFullName();
    uns32 from = 0, to = 0;

    // copy the filename to the file descriptor
    while( to < fname_len ) fname[to++] = str[from++];
    // append the null character to the filename if it is shorter than 8B
    if( to < FileNameSize-1 ) fname[to] = '\0';


    // skip the dot that separates the filename and extension
    from++; to = 0;


    // copy the extension to the file descriptor
    while( to < fext_len ) fname[to++] = str[from++];
    // append the null character to the extension if it is shorter than 3B
    if( to < FileExtSize-1 ) fname[to] = '\0';


    // return ok status code
    return MFS_OK;
}

// reset the file name fields to their defaults
void FileDescriptor::resetFullName()
{
    // clear the descriptor fields that hold the full filename       // [filename][ext]
    for( uns32 i = 0; i < FileNameSize - 1; i++ ) fname[i] = '\0';   //  <|        |
    for( uns32 i = 0; i < FileExtSize  - 1; i++ ) fext [i] = '\0';   //           <|
}



// print file descriptor to output stream
ostream& operator<<(ostream& os, const FileDescriptor& fd)
{
    // backup format flags and set fill character
    ios_base::fmtflags f(os.flags());
    char c = os.fill('\0');

    // create a buffer that will hold full filename, and fill it with the full filename
    char fullname[FullFileNameSize];
    fd.getFullName(fullname);

    // print the file descriptor fields
    os << setfill('-')
        << setw(FullFileNameSize-1) << left << fullname << right << ":fn "
        << hex
        << setw(uns32sz*bcw) << fd.filesize << ":B "
        << setw(uns32sz*bcw) << fd.indx     << ":i1   ";

    // print the first couple of bytes of the file in the file descriptor
    os << setfill('0') << hex;
    for( uns32 i = 1; i <= FileSizeS; i++ )
    {
        os << setw(uns8sz*bcw) << (uns32) fd.byte[i-1];
        if( i % 8 == 0 );
        else if( i % 2 == 0 ) os << ' ';
    }
    os << ":d";

    // restore format flags and fill character
    os.flags(f); os.fill(c);
    return os;
}



