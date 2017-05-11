/*********************                                                        */
/*! \file IFile.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __IFile_h__
#define __IFile_h__

class ConstSimpleData;
class String;
class HeapData;

class IFile
{
public:
    enum Mode {
        MODE_READ,
        MODE_WRITE_APPEND,
        MODE_WRITE_TRUNCATE,
    };

    virtual void open( Mode mode ) = 0;
    virtual void write( const String &line ) = 0;
    virtual void write( const ConstSimpleData &data ) = 0;
    virtual String readLine( char lineSeparatingChar = '\n' ) = 0;
    virtual void read( HeapData &buffer, unsigned maxReadSize ) = 0;
    virtual void close() = 0;

	virtual ~IFile() {}
};

#endif // __IFile_h__
