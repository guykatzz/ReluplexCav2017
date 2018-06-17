/*********************                                                        */
/*! \file File.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __File_h__
#define __File_h__

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Error.h"
#include "HeapData.h"
#include "IFile.h"
#include "MString.h"
#include <sys/stat.h>
#include <unistd.h>

class File : public IFile
{
public:
    File( const String &path ) : _path( path ), _descriptor( NO_DESCRIPTOR )
    {
    }

    ~File()
    {
        closeIfNeeded();
    }

    void close()
    {
        closeIfNeeded();
    }

    static bool exists( const String &path )
    {
        struct stat DONT_CARE;
        return stat( path.ascii(), &DONT_CARE ) == 0;
    }

    static bool directory( const String &path )
    {
        struct stat fileData;
        memset( &fileData, 0, sizeof(struct stat) );

        if ( stat( path.ascii(), &fileData ) == 0 )
            return S_ISDIR( fileData.st_mode );

        return false;
    }

    static unsigned getSize( const String &path )
    {
        struct stat dataBuffer;
        memset( &dataBuffer, 0, sizeof(dataBuffer) );

        if ( stat( path.ascii(), &dataBuffer ) != 0 )
            throw Error( Error::STAT_FAILED );

        return dataBuffer.st_size;
    }

    void open( Mode openMode )
    {
        int flags;
        mode_t mode;

        setParametersByMode( openMode, flags, mode );

        if ( ( _descriptor = ::open( _path.ascii(), flags, mode ) ) == NO_DESCRIPTOR )
            throw Error( Error::OPEN_FAILED );
    }

    void write( const String &line )
    {
        if ( ::write( _descriptor, line.ascii(), line.length() ) != (int)line.length() )
        {
            throw Error( Error::WRITE_FAILED );
        }
    }

    void write( const ConstSimpleData &data )
    {
        if ( ::write( _descriptor, data.data(), data.size() ) != (int)data.size() )
        {
            throw Error( Error::WRITE_FAILED );
        }
    }

    void read( HeapData &buffer, unsigned maxReadSize )
    {
        char readBuffer[maxReadSize];
        int bytesRead;

        if ( ( bytesRead = ::read( _descriptor, readBuffer, sizeof(readBuffer) ) ) == -1 )
            throw Error( Error::READ_FAILED );

        buffer = ConstSimpleData( readBuffer, bytesRead );
    }

    String readLine( char lineSeparatingChar = '\n' )
    {
        enum {
            SIZE_OF_BUFFER = 1024,
        };

        Stringf separatorAsString( "%c", lineSeparatingChar );

        while ( !_readLineBuffer.contains( separatorAsString ) )
        {
            char buffer[SIZE_OF_BUFFER + 1];
            int n = ::read( _descriptor, buffer, sizeof(char) * SIZE_OF_BUFFER );
            if ( ( n == -1 ) || ( n == 0 ) )
                throw Error( Error::READ_FAILED );

            buffer[n] = 0;
            _readLineBuffer += buffer;
        }

        String result = _readLineBuffer.substring( 0, _readLineBuffer.find( separatorAsString ) );
        _readLineBuffer = _readLineBuffer.substring( _readLineBuffer.find( separatorAsString ) + 1,
                                                     _readLineBuffer.length() );
        return result;
    }

private:
    enum {
        NO_DESCRIPTOR = -1,
    };

    String _path;
    int _descriptor;
    String _readLineBuffer;

    void closeIfNeeded()
    {
        if ( _descriptor != NO_DESCRIPTOR )
        {
            ::close( _descriptor );
            _descriptor = NO_DESCRIPTOR;
        }
    }

    void setParametersByMode( Mode openMode, int &flags, mode_t &mode )
    {
        if ( openMode == IFile::MODE_READ )
            flags = O_RDONLY;
        else if ( openMode == IFile::MODE_WRITE_TRUNCATE )
            flags = O_RDWR | O_CREAT | O_TRUNC;
        else
            flags = O_CREAT | O_RDWR | O_APPEND;

        if ( ( openMode == IFile::MODE_WRITE_APPEND ) || ( openMode == IFile::MODE_WRITE_TRUNCATE ) )
            mode = S_IRUSR | S_IWUSR;
        else
            mode = (mode_t)NULL;
    }
};

#endif // __File_h__
