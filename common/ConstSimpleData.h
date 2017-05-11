/*********************                                                        */
/*! \file ConstSimpleData.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __ConstSimpleData_h__
#define __ConstSimpleData_h__

#include "IConstSimpleData.h"
#include "IHeapData.h"
#include "Stringf.h"
#include <cstdio>

class ConstSimpleData : public IConstSimpleData
{
public:
	ConstSimpleData( const void *data, unsigned size ) : _data( data ), _size( size )
	{
	}

    ConstSimpleData( const IHeapData &data ) : _data( data.data() ), _size( data.size() )
    {
    }

	const void *data() const
	{
		return _data;
	}

	unsigned size() const
	{
		return _size;
	}

    const char *asChar() const
    {
        return (char *)_data;
    }

    void hexDump() const
    {
        for ( unsigned i = 0; i < size(); ++i )
        {
            printf( "%02x ", (unsigned char)asChar()[i] );
        }
        printf( "\n" );
        fflush( 0 );
    }

    String toString() const
    {
        String result;

        for ( unsigned i = 0; i < size(); ++i )
            result += Stringf( "%02x ", (unsigned char)asChar()[i] );

        return result;
    }

private:
	const void *_data;
	unsigned _size;
};

#endif // __ConstSimpleData_h__
