/*********************                                                        */
/*! \file HeapData.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Reluplex project.
 ** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
 ** (in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **/

#ifndef __HeapData_h__
#define __HeapData_h__

#include "ConstSimpleData.h"
#include "Error.h"
#include "IHeapData.h"
#include <stdlib.h>
#include <string.h>

class HeapData : public IHeapData
{
public:
	HeapData() : _data( 0 ), _size( 0 )
	{
	}

	HeapData( void *data, unsigned size ) : _data( 0 ), _size( 0 )
	{
		allocateMemory( size );
		memcpy( _data, data, size );
	}

    HeapData( HeapData &other ) : _data( 0 ), _size( 0 )
    {
        ( *this )+=other;
    }

    HeapData( const IConstSimpleData &constSimpleData )
    {
        allocateMemory( constSimpleData.size() );
        memcpy( _data, constSimpleData.data(), constSimpleData.size() );
    }

    HeapData( const HeapData &other )
    {
        allocateMemory( other.size() );
        memcpy( _data, other.data(), other.size() );
    }

	~HeapData()
	{
		freeMemoryIfNeeded();
	}

	HeapData &operator=( const IConstSimpleData &data )
	{
        freeMemoryIfNeeded();
		allocateMemory( data.size() );
		memcpy( _data, data.data(), data.size() );
		return *this;
	}

	HeapData &operator=( const HeapData &other )
	{
		freeMemoryIfNeeded();
		allocateMemory( other.size() );
		memcpy( _data, other.data(), other.size() );
		return *this;
	}

    HeapData &operator+=( const IConstSimpleData &data )
    {
        addMemory( data.size() );
        copyNewData( data.data(), data.size() );
        adjustSize( data.size() );

        return *this;
    }

    HeapData &operator+=( const IHeapData &data )
    {
        ( *this )+=( ConstSimpleData( data ) );
        return *this;
    }

    bool operator==( const HeapData &other ) const
    {
        if ( _size != other._size )
            return false;

        return ( memcmp( _data, other._data, _size ) == 0 );
    }

    bool operator!=( const HeapData &other ) const
    {
        return !( *this == other );
    }

    bool operator<( const HeapData &other ) const
    {
        return
            ( _size < other._size ) ||
            ( ( _size == other._size ) && ( memcmp( _data, other._data, _size ) < 0 ) );
    }

	void *data()
	{
		return _data;
	}

	const void *data() const
	{
		return _data;
	}

	unsigned size() const
	{
		return _size;
	}

    void clear()
    {
        freeMemoryIfNeeded();
    }

    const char *asChar() const
    {
        return (char *)_data;
    }

private:
	void *_data;
	unsigned _size;

	bool allocated() const
	{
		return _data != NULL;
	}

	void freeMemory()
	{
		free( _data );
		_data = NULL;
        _size = 0;
	}

	void freeMemoryIfNeeded()
	{
		if ( allocated() )
		{
			freeMemory();
		}
	}

	void allocateMemory( unsigned size )
	{
		_size = size;

		if ( ( _data = malloc( _size ) ) == NULL )
		{
			throw Error( Error::NOT_ENOUGH_MEMORY );
		}
	}

    void addMemory( unsigned size )
    {
        if ( ( _data = realloc( _data, _size + size ) ) == NULL )
        {
            throw Error( Error::NOT_ENOUGH_MEMORY );
        }
    }

    void copyNewData( const void *newData, unsigned size )
    {
        memcpy( (char *)_data + _size, newData, size );
    }

    void adjustSize( unsigned size )
    {
        _size += size;
    }
};

#endif // __HeapData_h__
